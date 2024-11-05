# sc-machine

[![CI](https://github.com/ostis-ai/sc-machine/actions/workflows/main.yml/badge.svg)](https://github.com/ostis-ai/sc-machine/actions/workflows/main.yml)
[![codecov](https://codecov.io/gh/ostis-ai/sc-machine/branch/main/graph/badge.svg?token=WU8O9Z1DNL)](https://codecov.io/gh/ostis-ai/sc-machine)
[![license](https://img.shields.io/badge/License-MIT-yellow.svg)](COPYING.MIT)
[![docker](https://img.shields.io/docker/v/ostis/sc-machine?arch=amd64&label=Docker&logo=Docker&sort=date)](https://hub.docker.com/r/ostis/sc-machine)

Short version: **sc-machine** is a software package of Semantic network storage that emulates semantic computer behaviour. It uses agent-based approach to process knowledge graphs.

Semantic network storage stores and processes knowledge graphs represented in the SC-code (language of the universal knowledge representation). Theoretical basis of sc-machine is provided by the [**OSTIS Technology**](https://github.com/ostis-ai).

Semantic network storage allows integrating problem solutions from different subject domains **by using the same**:

- **technology**
- **programming and data representation language**
- **software stack**.

This project contains:

- `sc-memory` - semantic storage to store graph constructions;
- `sc-event-manager` - event-based processing (async program workflow handling);
- tools to communicate with `sc-memory`:
  - native C++ API;
  - `sc-server` - a Websocket server that provides a network API to communicate with storage using JSON;
  - `sc-builder` loads information from SCs files into the storage (SCs-code is one of the external representation languages of the SC-code).

<details>
   <summary>More info</summary>

Sc-machine is a **platform-independent graph database management system** that can store / retrieve knowledge graphs and run tasks (agents) on them.

Both declarative (data, data structures, documentation, tasks specification, etc.) and procedural
(programs, modules, systems, communication between systems) knowledge is represented using the same language: the SC-code.

</details>

## Documentation

- A brief user manual and developer docs are hosted on our [GitHub Pages](https://ostis-ai.github.io/sc-machine).
- Full documentation, including:

  - core concepts
  - rationale behind the sc-machine
  - system design
  - software interfaces

  is redistributed in a form of the [SCn-TeX document](https://github.com/ostis-ai/ostis-web-platform/blob/develop/docs/main.pdf).

  or **alternatively** you can build sc-machine documentation only. To do that refer to the [scn-latex-plugin](https://github.com/ostis-ai/scn-latex-plugin) documentation.

<details>
  <summary>Build documentation locally</summary>

```sh
# make sure you're using python12
pip3 install mkdocs mkdocs-material
mkdocs serve
# and open http://127.0.0.1:8000/ in your browser
```
</details>

## Quick start

Semantic network storage is a core of any ostis-system, so you can use a reference system named [OSTIS Platform](https://github.com/ostis-ai/ostis-web-platform) to get it up and running quickly.

## Installation

- Docker:
  We provide a Docker image for this project. Head to [Installing with Docker](https://ostis-ai.github.io/sc-machine/docker) to learn more. It's the recommended way to deploy the sc-machine.
- Native:
  If you do not have the option to deploy the system using Docker, please refer to the docs:
  [Build system](https://ostis-ai.github.io/sc-machine/build/build_system/)

  Note: currently, the sc-machine isn't _natively_ supported on Windows.

## Usage

- Docker

  ```sh
  # create empty KB sources folder
  mkdir kb && cat "." > kb/repo.path
  # note: at this stage you can move your KB sources to the ./kb folder
  
  # build kb
  docker compose run --rm machine build
  # run sc-machine
  docker compose up
  ```

- Native

  Note: Currently, only Linux (Ubuntu-20.04, Ubuntu-22.04, Ubuntu-24.04) and macOS are supported by this installation method. If you're going to use it, it might take a while to download dependencies and compile the components. Use it only if you know what you're doing!

  ```sh
  # build kb
  ./build/<Debug|Release>/bin/sc-builder -c ./sc-machine.ini -i <path to kb folder with SCs and SCg sources (or path to repo.path file)> -o <output path> --clear
  # launch sc-machine
  ./build/<Debug|Release>/bin/sc-machine -c ./sc-machine.ini
  ```

Most of these commands have a help page bundled inside, so if you have any questions or want to customize the command behavior, use `--help` flag to learn more about them.

## Config

This repository provides a default configuration for the sc-machine. To customize the _sc-machine_ to suit your needs you can [create your own config file](https://ostis-ai.github.io/sc-machine/build/config).

## Feedback

Contributions, bug reports and feature requests are welcome!
Feel free to check our [issues page](https://github.com/ostis-ai/sc-machine/issues) and file a new issue (or comment in existing ones).

## License

Distributed under the MIT License. Check [COPYING.MIT](COPYING.MIT) for more information.

##### _This repository continues the development of [ostis-dev/sc-machine](https://github.com/ostis-dev/sc-machine) from version 0.6.0._
