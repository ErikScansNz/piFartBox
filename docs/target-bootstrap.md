# Target Bootstrap

This note records the first observed live target facts for the current Raspberry Pi deployment target.

## Target
- host: `192.168.1.26`
- hostname: `zynthian`
- user: `pi`

## Observed Runtime Facts
- kernel: `Linux zynthian 4.14.30+ #1102 Mon Mar 26 16:20:05 BST 2018 armv6l GNU/Linux`
- OS: `Raspbian GNU/Linux 9 (stretch)`
- architecture: `armv6l`
- active service: `zynthian.service`

## Observed Installed Tools
- `git`: present at `/usr/bin/git`
- `cmake`: present at `/usr/bin/cmake`
- `g++`: present at `/usr/bin/g++`

Observed versions:
- `cmake`: `3.7.2`
- `g++`: `6.3.0`

## Existing Zynthian Layout
- active Zynthian trees exist under:
  - `/zynthian`
  - `/home/pi`
- current bootstrap work must not modify the existing `/zynthian` runtime tree

## Bootstrap Default
- new project install root: `/opt/piFartBox`
- transfer model: SSH/SFTP + git bundle
- remote staging root: `/home/pi/piFartBox-update`

## First Bootstrap Outcome
- initial repository bootstrap to `/opt/piFartBox` succeeded
- the deployed revision matches the current local source revision
- first configure/build validation is currently blocked by host toolchain age:
  - the repo currently requires `cmake >= 3.20`
  - the target currently provides `cmake 3.7.2`
  - the target compiler is `g++ 6.3.0`, which is below the expected modern C++ baseline

This means the next concrete implementation task is target toolchain compatibility, not deeper engine work on the Pi yet.
