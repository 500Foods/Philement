# Lithium

The main TOC for the Lithium project can be found in
[**/docs/Li/README.md**](/docs/Li/README.md). But since you're here, these might
be of interest to you.

## Project Structure

- [**src/**](/elements/003-lithium/src/README.md) - Main JavaScript source code
  with ES6 modules
  - [app.js](/elements/003-lithium/src/app.js) - Main application entry point
  - [router/](/elements/003-lithium/src/router/) - SPA routing module
  - [storage/](/elements/003-lithium/src/storage/) - Data storage with IndexedDB
  - [network/](/elements/003-lithium/src/network/) - Network and API module
  - [utils/](/elements/003-lithium/src/utils/) - Utility modules

- [**tests/**](/elements/003-lithium/tests/README.md) - Comprehensive test suite
  with Mocha + nyc
  - [test-runner.js](/elements/003-lithium/tests/test-runner.js) - Test
    orchestration script
  - [tests/](/elements/003-lithium/tests/tests/) - Individual test files

- [**dist/**](/elements/003-lithium/dist/README.md) - Production build output
  - [index.html](/elements/003-lithium/dist/index.html) - Minified HTML
  - Production-ready assets

- [**assets/**](/elements/003-lithium/assets/) - Static assets
  - [fonts/](/elements/003-lithium/assets/fonts/) - Web fonts (Vanadium, Roboto)
  - [images/](/elements/003-lithium/assets/images/) - App images and logos
  - [icons/](/elements/003-lithium/assets/icons/) - PWA icons for all platforms

## Key Files

- [**package.json**](/elements/003-lithium/package.json) - Project dependencies
  and scripts
- [**index.html**](/elements/003-lithium/index.html) - Development HTML entry point
- [**lithium.css**](/elements/003-lithium/lithium.css) - Main stylesheet with
  Vanadium fonts
- [**manifest.json**](/elements/003-lithium/manifest.json) - PWA manifest
- [**service-worker.js**](/elements/003-lithium/service-worker.js) - Service
  worker for offline support

## Testing Framework

- [**Mocha**](https://mochajs.org/) - JavaScript test framework
- [**Chai**](https://www.chaijs.com/) - BDD/TDD assertion library
- [**NYC**](https://github.com/istanbuljs/nyc) - Code coverage tool
- [**Sinon**](https://sinonjs.org/) - Test spies, stubs and mocks

## Documentation

- [**Full Documentation**](/docs/Li/README.md) - Complete project documentation
- [**Testing Guide**](/docs/Li/TESTING.md) - Testing framework and procedures
- [**API Reference**](/docs/Li/API.md) - API documentation (coming soon)

## License

- [**LICENSE**](/docs/Li/LICENSE.md) - This project is covered by the MIT License
