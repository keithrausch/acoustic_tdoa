# WORK IN PROGRESS

- This project is a long running hobby to build an acoustic, rigid-body motion capture suite using speakers and microphones

```text
.
├── embedded (done. code to run on teensy for doing 4+ channel TDOA, including M7 and x86 unit tests)
├── latex (derivations and presentations)
└── sandbox (experiments and incomplete efforts)
    ├── correlation (feasibility tests for FFT based signal detection and subsample registration)
    ├── signallock (basically trash)
    └── trilateration (WIP effort to learn React. lots of AI)
```

### Setup
- `embedded`: cmake based and uses CMakePresets. see the `embedded/scripts` to setup, bulid, and flash the teensy
- `sandbox/trilateration`: ehhh
- the rest is misc python scripts

### AI Usage
- `embedded`: never for algorithm development or math. mostly just to adapt non-arduino style libraries to teensy-cmake-macros
- `sandbox/trilateration`: heavy use for React


