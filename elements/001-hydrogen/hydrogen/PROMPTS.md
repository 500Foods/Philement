# Prompts

Using AI models at this point in time, late 2025, sometimes can be a tricky business. They're smart and capable enough to accomplish neary any task. But they have the attention span and behaviour of a rambunctious little 5 year old child. Can be taxing at times. Generally, the approach is to give the model a task and only a few turns at getting it right before it becomes too bogged down to be of any use to anyone. Longer context windows helped initally, particularly when they were less than 200,000 tokens, but now that we're past that, it is just more fodder for confusion. Somewhat unexpected perhaps, but here we are.  

What follows then are prompts that have been used to get things done, generally refined after many attempts that ended up with less than positive outcomes.

## Writing Unit Tests

Hyrogen project is in elements/001-hydrogen/hydrogen folder.
Everything we're doing here assumes this as the project root.
For example,  tests/ is in elements/001-hydrogen/hydrogen/tests

Review RECIPE.md.
Review tests/README.md
Review tests/UNITY.md
Review tests/unity/mocks/README.md

We're working on improving overall unit test coverage for our C project. For Unity Framework unit tests, the primary goal is to increase our test coverage over what we have from existing Unity and Blackbox tests.  If code is already covered by one or both of those, we don't really need to write new unit tests for it.

For a file in src/path/to/file.c, the Blackbox test coverage can be found in buld/coverage/src/path/to/file.c.gcov.  
Unity test coverage can be found in build/unity/src/path/to/file.c.gcov.

When building a brand new Unity unit test .c file, you must run `mkt` to perform an initial trial build. Correct any errors found before continuing. After a trial build is successful, use `mku file` (wihtout the .c or the path) to buld and run the individual tests for subsequent development cycles. This will recompile and run the individual tests file each time.

Some additional test-writing advice:

- Tests may uncover bugs in the underlying source code. That's fantastic! Don't be shy about fixing bugs that the unit tests uncover.
- Tests should be all passing when they are first written. If a condition is expected, the test should pass if that condition is encountered. If we just wrote a test that is failing, then we can either fix the test to expect the condition we're encountering, or we can fix the underlying code or mock library so that it produces the expected results. Either is a valid approach.
- Be sure to follow carefully the guidelines listed in UNITY.md, particularly around file naming conventions. We need these named this way so that our build system reports on them properly.
- Pay close attention to the mocks README.md if you need or want to use mock libraries. Test resources include mock libraries for system functions like libwebsockets and microhttpd in tests/unity/mocks, along with many more mock libraries for our own code.
- src/config/config_defaults.c available for setting up and using app_config if a test needs it.
- If a source file contains static functions, generally we want to change them to be non-static so that we can write unit tests against them.
- We have thousands of tests across hundreds of individual test files, so it is likely you can find examples of something if you're stuck somewhere. Search for a function or mock library or whatever you're trying to use, and most liekly you'll come up with a list of examples where it has been used previously.
- We're after unit tests that are small, quick to execute, and that are focused on a single task. We run thousdands of these all in parallel, so bing quick and failure-free are key objectives.

Once a test has been written, builds cleanly, and has all of its parts passing, use cppcheck (run test 91 or just usee the `mkp` command) to see if there are any issues to address. As with the build, we don't have any tolerance for errors or warnings of any kind. Sometimes they cannot be corrcted because the function signature is required to be a particular way, in which case we can add a cppcheck exclusion comment, but this should be a last resort and should include a justification as to why this is the only viable option rather than fixing the code.

Often we're struggling to reach our coverage targets. For source files with > 100 lines of instrumented code, we require > 75% coverage. For source files with < 100 lines of code we require > 50% coverage. To achieve our coverage targets we have a few options.

- As wee want actual coverage, we want to call funcitons direectly whenever possible. Routinely, this will involve splitting up longer functions with helper functions that can be easier to test. For large files, we can break them up into smaller fils if it makes sense to do so. Somtimes wee can use this to get under 100 lines and thus lower the coverage threshold.
- We can also refactor the code as there is likely to be a lot of repetittive code across functions, which can hlep reduce the amount of code overall.
- And, importantly, our ovrall covreage will only increase if we write tests for code that isn't already covered by existing unit tests or by our black box coverage, so be sure to check both of those gcov files to identify coverage opportunities. To make this unambiguous and simple, a new extras/add_coverage.sh script has been created, showing the uncovered lines in both of the two files passed in as parameters.

Current covreage target is at 72 %
src/api/conduit/query/query.c

View missing coverage:
cd elements/001-hydrogen/hydrogen && ./extras/add_coverage.sh build/unity/src/api/conduit/query/query.c.gcov build/coverage/src/api/conduit/query/query.c.gcov
