# Unity Test Framework for Hydrogen Project

## Overview of Unity

Unity is a lightweight, portable, and highly configurable unit testing framework designed specifically for embedded C development. It is particularly well-suited for testing C code in constrained environments, such as microcontrollers or other resource-limited systems. Unity provides a simple yet powerful way to write and run unit tests, enabling developers to validate individual components of their codebase in isolation.

Key features of Unity include:

- **Minimal Footprint**: Designed to run in environments with limited memory and processing power.
- **Easy Integration**: Can be easily integrated into existing build systems and development workflows.
- **Flexible Assertions**: Offers a wide range of assertion macros to check for various conditions (e.g., equality, inequality, and custom conditions).
- **Test Isolation**: Ensures tests are independent, reducing the risk of test interference.
- **Mocking Support**: Works well with mocking frameworks like CMock for simulating dependencies.

In the context of the Hydrogen project, Unity is used to create and execute unit tests to verify the functionality of core components, ensuring reliability and correctness of the system.

## Installation Instructions for Unity Framework

The Unity framework files are not included in this repository due to their size and to respect licensing considerations. They are listed in the `.gitignore` file to prevent accidental inclusion. To run the unit tests for the Hydrogen project, you must download and install the Unity framework into the `unity/framework` directory within this project structure.

Follow these steps to install the Unity framework:

### Prerequisites

- Ensure you have `curl` or a similar tool installed on your system to download files from the internet.
- Make sure you are in the root directory of the Hydrogen project (`elements/001-hydrogen/hydrogen/`).

### Steps to Install Unity Framework

1. **Create the Framework Directory** (if it doesn't already exist):

   ```bash
   mkdir -p tests/unity/framework
   ```

2. **Download the Latest Unity Framework**:
   The Unity framework is hosted on GitHub by ThrowTheSwitch. You can download the latest release or clone the repository. Below is a simple command to download the master branch as a zip file using `curl`:

   ```bash
   curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o tests/unity/framework/unity.zip
   ```

3. **Extract the Downloaded Files**:
   After downloading, extract the contents of the zip file into the `tests/unity/framework` directory, ensuring the Unity framework is placed in `tests/unity/framework/Unity/`:

   ```bash
   unzip tests/unity/framework/unity.zip -d tests/unity/framework/
   mv tests/unity/framework/Unity-master tests/unity/framework/Unity
   rm tests/unity/framework/unity.zip
   ```

4. **Verify Installation**:
   Check that the Unity framework files are correctly placed in `tests/unity/framework/Unity/`. You should see files like `unity.h`, `unity.c`, and `unity_internals.h` among others in the `Unity` subdirectory.

### Alternative: Cloning the Repository

If you prefer to clone the repository directly (requires `git` to be installed):

```bash
git clone https://github.com/ThrowTheSwitch/Unity.git tests/unity/framework/Unity
```

### Notes

- Ensure that your build system or test scripts are configured to reference the Unity framework files in `tests/unity/framework/Unity/`.
- If a specific version of Unity is required for compatibility with the Hydrogen project, consult the project documentation or maintainers for the exact version or commit hash to use.

## Running Unity Tests

Once the Unity framework is installed, you can build and run the unit tests for the Hydrogen project. Refer to the project's testing documentation (`docs/testing.md`) for detailed instructions on compiling and executing the tests.

If you encounter issues with the installation or test execution, please reach out to the project maintainers for assistance.
