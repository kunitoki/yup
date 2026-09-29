# Contributing to yup

Thanks for helping improve yup! This page covers what we accept, how to build, and how to submit a change. If anything is unclear, [open an issue](https://github.com/kunitoki/yup/issues) and ask.

Please read our [Code of Conduct](./CODE_OF_CONDUCT.md) first.

## What we accept

**Welcome:** bug fixes, build and platform fixes, performance improvements with measurements, useful new examples, and documentation fixes.

**Ask in an issue first:** changes to the public API, new dependencies, and large changes.

**Not accepted:** style-only edits, refactors or renames with no functional reason, and niche personal-preference changes.

## Build yup

Check the [README](../README.md) and CI configuration for current prerequisites. The basic steps are:

```bash
git clone --recursive https://github.com/kunitoki/yup.git
cd yup
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run the examples and tests before you change anything, so you know your setup works.

## Make a change

1. Fork the repository and create a branch off `main`, such as `fix/rhi-texture-leak`.
2. Make your change. Match the style of the surrounding code, and add tests for behavior changes.
3. Build and run the tests.
4. Commit with a clear message that says what changed and why.
5. Push and open a pull request.

For small documentation fixes, you can click **Edit** on the file in GitHub instead.

## Open a pull request

- Fill in the template and link the related issue (for example, `Fixes #123`).
- Say what you tested and on which platforms, including any you couldn't test.
- Keep it focused. Put unrelated fixes in a separate pull request.
- Enable maintainer edits so reviewers can push small fixes.

Reviews can take a while. A polite ping after a week is fine.

## Using AI tools

AI tools are welcome, with three rules:

- **Understand it.** You must be able to explain every line you submit.
- **Test it.** Build and run AI-generated code yourself before opening a pull request.
- **Don't credit the AI.** Commits with an `Author:` or `Co-authored-by:` line naming an AI or bot won't be merged. Many tools add these automatically, so turn that off.

## Report an issue

Search existing issues first. For bugs, include your OS, compiler version, yup commit, the command you ran, and a minimal reproduction. For build failures, paste the full CMake output.
