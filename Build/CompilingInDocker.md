## Docker builds

Docker builds are managed from the `Makefile` in the root folder. Typing `make` will print out some helpful information.

### Prerequisites

Required:

- `docker`
- `docker-compose`
- `Makefile`

Recommende:

- `vs code`

### Build Image

`iSEG` can be build entirely using a docker build image based on ubuntu 18.04. The build image is published on [Dockerhub](https://hub.docker.com/repository/docker/itisfoundation/iseg-ubuntu-buildkit).
The build image contains all required 3rd party dependencies and compilers to build `iSEG`.
If required, the build image can be rebuild using the `Dockerfile` in the `docker` subfolder.

To create a new one:

```
make create-build-image
```

To publish it to the `Dockerhub`:

```
make publish-build-image.
```

Make sure to increase the version for every new build image.

### Compiling

Compilation is done by mounting the source folder into the build container and executing a build script. Compiled binaries are kept in a docker-volume to avoid recompilation after every build invocation.

To compile in release:

```
make build-release
```

To compile in debug:

```
make build-debug
```

To clean the build folders (i.e. removing the build volume)

```
make clean-build
```

### Debugging

Visual Studio Codes remote container debugging can be used as follows:

```
make debug-in-container
```

This creates a `devcontainer` settings and starts a new vs code instance using the build-image. By default, the `iSeg` debug target is active. New ones can be added by editing a `launch.json` file.

### Run

`iSeg` can be started via

```
make run-release
```

or

```
make run-debug
```

as well as from the debug container. This is achieved by mounting the `X11` socket and the `DISPLAY` variable into the build containers.
