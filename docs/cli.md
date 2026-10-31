# CLI

The CLI binary is built as `entroui` in the build directory (CMake target `entroui-cli`).

```bash
./build/debug/entroui help
```

## Create a project

```bash
./build/debug/entroui create my-app
```

Generated structure:

```text
my-app/
├── CMakeLists.txt
├── conanfile.py
├── src/main.cpp
├── ui/app.eui
├── ui/styles.css
├── assets/
└── README.md
```

The generated project uses the local EntroUI source tree when the `ENTROUI_ROOT`
environment variable points to it. The CLI sets this automatically when it was
built from this repository. Otherwise the project expects an installed
`EntroUI` CMake package.

## Build and run

```bash
./build/debug/entroui build my-app
./build/debug/entroui run my-app
```

`build` configures with CMake/Ninja into `my-app/build` and compiles the `app`
target. `run` launches the compiled binary with the project directory as its
working directory.

## Test

```bash
./build/debug/entroui test my-app
```

Runs CTest in the project's build directory. Generated projects currently do not
include tests by default.

## Check markup

```bash
./build/debug/entroui check ui/
./build/debug/entroui check ui/app.eui
```

Checks every `.eui` file under a directory (recursively) or the listed files. It
prints parser and builder diagnostics with file/line/column and exits non-zero
when any error is found.

## Version

```bash
./build/debug/entroui version
```

## Exit codes

| Code | Meaning |
|---|---|
| 0 | Success |
| 1 | Build/run/check failure |
| 2 | Invalid usage or arguments |
