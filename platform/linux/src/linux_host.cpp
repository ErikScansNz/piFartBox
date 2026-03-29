#include "pi_fartbox/platform/linux_host.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <numbers>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__) && __has_include(<alsa/asoundlib.h>)
#define PI_FARTBOX_HAVE_ALSA 1
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#else
#define PI_FARTBOX_HAVE_ALSA 0
#endif

namespace pi_fartbox::platform {

namespace {

using namespace std::chrono_literals;

auto trim(std::string value) -> std::string {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

}  // namespace

LinuxHost::LinuxHost(LinuxHostConfig config) : config_(std::move(config)), audio_probe_(probe_audio_state()) {}

auto LinuxHost::config() const noexcept -> const LinuxHostConfig& {
  return config_;
}

auto LinuxHost::subsystem_name() const noexcept -> std::string_view {
  return "platform/linux";
}

auto LinuxHost::audio_probe() const -> const AudioProbeInfo& {
  return audio_probe_;
}

auto LinuxHost::probe_audio_state() -> AudioProbeInfo {
  AudioProbeInfo info;

  const auto cards_path = std::filesystem::path("/proc/asound/cards");
  const auto pcm_path = std::filesystem::path("/proc/asound/pcm");
  info.proc_asound_available = std::filesystem::exists(cards_path) || std::filesystem::exists(pcm_path);

  if (std::ifstream cards(cards_path); cards.is_open()) {
    std::string line;
    while (std::getline(cards, line)) {
      if (!line.empty() && std::isdigit(static_cast<unsigned char>(line.front()))) {
        info.cards.push_back(trim(line));
      }
    }
  }

  if (std::ifstream pcm(pcm_path); pcm.is_open()) {
    std::string line;
    while (std::getline(pcm, line)) {
      const auto cleaned = trim(line);
      if (!cleaned.empty()) {
        info.pcm_devices.push_back(cleaned);
      }
    }
  }

  return info;
}

class AlsaPlaybackEngine::Impl {
 public:
  explicit Impl(LinuxHostConfig config) : config_(std::move(config)) {
    status_.compiled_with_alsa = PI_FARTBOX_HAVE_ALSA != 0;
    status_.requested_device = config_.alsa_device;
    status_.tone_enabled = config_.audio_test_tone_enabled;
    status_.tone_frequency_hz = config_.audio_test_tone_hz;
    status_.sample_rate_hz = config_.audio_sample_rate_hz;
    status_.channels = config_.audio_channels;
    status_.period_frames = config_.audio_period_frames;
    status_.period_count = config_.audio_period_count;
  }

  ~Impl() {
    stop();
  }

  auto start() -> bool {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (thread_.joinable()) {
        return status_.device_opened;
      }

      stop_requested_ = false;
      startup_complete_ = false;
    }

    thread_ = std::thread([this] { run(); });

    std::unique_lock<std::mutex> startup_lock(startup_mutex_);
    startup_cv_.wait_for(startup_lock, 5s, [this] { return startup_complete_; });
    std::lock_guard<std::mutex> lock(mutex_);
    return status_.device_opened;
  }

  auto stop() -> void {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_requested_ = true;
    }

    if (thread_.joinable()) {
      thread_.join();
    }
  }

  [[nodiscard]] auto status() const -> AlsaPlaybackStatus {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
  }

 private:
  auto run() -> void {
#if PI_FARTBOX_HAVE_ALSA
    snd_pcm_t* pcm = nullptr;
    auto notify_startup = [this] {
      std::lock_guard<std::mutex> startup_lock(startup_mutex_);
      startup_complete_ = true;
      startup_cv_.notify_all();
    };

    {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.last_error.clear();
    }

    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.memory_locked = true;
    }

    sched_param scheduling{};
    scheduling.sched_priority = static_cast<int>(config_.audio_realtime_priority);
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &scheduling) == 0) {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.realtime_thread = true;
    } else {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.last_error = "Unable to elevate audio thread to SCHED_FIFO";
    }

    if (const auto error = snd_pcm_open(&pcm, config_.alsa_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0); error < 0) {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.last_error = std::string("snd_pcm_open: ") + snd_strerror(error);
      notify_startup();
      return;
    }

    snd_pcm_hw_params_t* hw_params = nullptr;
    snd_pcm_hw_params_alloca(&hw_params);

    int error = snd_pcm_hw_params_any(pcm, hw_params);
    if (error >= 0) {
      error = snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    }
    if (error >= 0) {
      error = snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    }

    unsigned int sample_rate = config_.audio_sample_rate_hz;
    if (error >= 0) {
      error = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &sample_rate, nullptr);
    }

    unsigned int channels = config_.audio_channels;
    if (error >= 0) {
      error = snd_pcm_hw_params_set_channels_near(pcm, hw_params, &channels);
    }

    snd_pcm_uframes_t period_frames = config_.audio_period_frames;
    if (error >= 0) {
      error = snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &period_frames, nullptr);
    }

    unsigned int period_count = config_.audio_period_count;
    if (error >= 0) {
      error = snd_pcm_hw_params_set_periods_near(pcm, hw_params, &period_count, nullptr);
    }

    if (error >= 0) {
      error = snd_pcm_hw_params(pcm, hw_params);
    }
    if (error >= 0) {
      error = snd_pcm_prepare(pcm);
    }

    if (error < 0) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.last_error = std::string("ALSA setup failed: ") + snd_strerror(error);
      }
      snd_pcm_close(pcm);
      notify_startup();
      return;
    }

    snd_pcm_hw_params_get_rate(hw_params, &sample_rate, nullptr);
    snd_pcm_hw_params_get_channels(hw_params, &channels);
    snd_pcm_hw_params_get_period_size(hw_params, &period_frames, nullptr);
    snd_pcm_hw_params_get_periods(hw_params, &period_count, nullptr);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.device_opened = true;
      status_.thread_running = true;
      status_.active_device = snd_pcm_name(pcm) ? snd_pcm_name(pcm) : config_.alsa_device;
      status_.sample_rate_hz = sample_rate;
      status_.channels = channels;
      status_.period_frames = static_cast<std::uint32_t>(period_frames);
      status_.period_count = period_count;
      if (status_.last_error == "Unable to elevate audio thread to SCHED_FIFO") {
        status_.last_error += " (continuing with normal scheduling)";
      } else {
        status_.last_error.clear();
      }
    }
    notify_startup();

    std::vector<std::int16_t> buffer(static_cast<std::size_t>(period_frames) * channels, 0);
    double phase = 0.0;
    const auto phase_increment =
        (config_.audio_test_tone_hz * 2.0 * std::numbers::pi_v<double>) / std::max(1u, sample_rate);

    while (true) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_requested_) {
          break;
        }
      }

      if (config_.audio_test_tone_enabled) {
        for (std::size_t frame = 0; frame < static_cast<std::size_t>(period_frames); ++frame) {
          const auto sample = static_cast<std::int16_t>(
              std::sin(phase) * config_.audio_test_tone_level * static_cast<double>(std::numeric_limits<std::int16_t>::max()));
          phase += phase_increment;
          if (phase >= 2.0 * std::numbers::pi_v<double>) {
            phase -= 2.0 * std::numbers::pi_v<double>;
          }
          for (unsigned int channel = 0; channel < channels; ++channel) {
            buffer[frame * channels + channel] = sample;
          }
        }
      } else {
        std::fill(buffer.begin(), buffer.end(), 0);
      }

      auto frames_written = snd_pcm_writei(pcm, buffer.data(), period_frames);
      if (frames_written < 0) {
        if (frames_written == -EPIPE) {
          std::lock_guard<std::mutex> lock(mutex_);
          ++status_.xrun_count;
        }
        frames_written = snd_pcm_recover(pcm, static_cast<int>(frames_written), 1);
        if (frames_written < 0) {
          std::lock_guard<std::mutex> lock(mutex_);
          status_.last_error = std::string("snd_pcm_writei: ") + snd_strerror(static_cast<int>(frames_written));
          std::this_thread::sleep_for(10ms);
        }
        continue;
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        ++status_.render_cycle_count;
      }
    }

    snd_pcm_drop(pcm);
    snd_pcm_close(pcm);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.thread_running = false;
    }
#else
    {
      std::lock_guard<std::mutex> lock(mutex_);
      status_.last_error = "ALSA headers/libs not available at build time";
    }
    {
      std::lock_guard<std::mutex> startup_lock(startup_mutex_);
      startup_complete_ = true;
      startup_cv_.notify_all();
    }
#endif
  }

  LinuxHostConfig config_;
  mutable std::mutex mutex_;
  mutable std::mutex startup_mutex_;
  std::condition_variable startup_cv_;
  std::thread thread_;
  AlsaPlaybackStatus status_;
  bool stop_requested_ = false;
  bool startup_complete_ = false;
};

AlsaPlaybackEngine::AlsaPlaybackEngine(LinuxHostConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

AlsaPlaybackEngine::~AlsaPlaybackEngine() = default;

auto AlsaPlaybackEngine::start() -> bool {
  return impl_->start();
}

auto AlsaPlaybackEngine::stop() -> void {
  impl_->stop();
}

auto AlsaPlaybackEngine::status() const -> AlsaPlaybackStatus {
  return impl_->status();
}

}  // namespace pi_fartbox::platform
