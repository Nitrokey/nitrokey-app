# How to contribute

## Ideas
If you would like to contribute and do not have the idea, you can check our [Ideas](https://github.com/Nitrokey/wiki/wiki/Ideas) wiki page.

## Reporting issues
### What issue description should contain
To increase issue report processing please add some information about your environment. At least:
- Operating system (name, version, 32 or 64 bits?)
- Stick firmware version (could be checked via `About` window)
- Application version on which the issue occurs
- Any available output or log (could be gathered from `Debug` window, found in tray menu when run with `-d` switch as in  `./nitrokey-app -d`) attached as a *text* file

In future this might be replaced by submitting one report file produced by application. A detailed proposition for data being gathered is under issue #74 [link](https://github.com/Nitrokey/nitrokey-app/issues/74).

## Pull request / commits content
### General guidelines
Whether you have a solution to one of the issues or a quick small fix-up for a typo you are always welcomed to submit a Pull Request (PR). Before submitting please check following guidelines:
- Make as small consisting changes as possible within one PR/commit (eg. do not mix translations and functional changes). Mixed PRs/commits might conflict with each other and thus delay or make impossible to merge them. Unclear changes might also be not merged. Please do code reformat with separate commit (last) if not done before each commit.
- Before commiting please run `./coding-styles.sh` (requires `clang-format` installed) to reformat all modified files.
- Please do not squash changes to one commit. If one of the changes would need to be reverted, it would not be possible without reverting whole commit or editing code by hand.
- Do not mix functional changes with not own code reformat. This is a straight road for merge conflicts and problems with reverting commits around, based on this one.
- Please sign-off your commit using `-s` switch like in `git commit -s` to confirm that you are the author of the patch or have the right to pass it on as open source (details: http://developercertificate.org/). This rule makes sense only when you are using your real name. You can also sign it with your GPG key if you would like to.
- Travis build system will check is code formatted and builds properly. Later on it will check also all warnings emitted during compilation.
- Make sure your code is synced with the `master` branch. We could ask you to rebase it in other case.
- Prefer extending current solutions over introducing foreign ones where possible.
- Please comment solution code in pull request and the impact of planned change on issue page.
- Add test scenario to PR under which the solution fixes the issue (with environment description).

Please be aware not all PRs will be merged to codebase including yours, however your effort will not go in vain. The PR could shed light on some overlook problem or be used as an inspiration for rewriting given code later. You can always develop it further in your fork and try again. Don't get discouraged!

### How to make a Pull Request
To make a PR please:

1. Make a fork of the repository
2. Create new branch based on master
3. Commit your code there and push to your forked repository
4. On main page of your forked repository a button creating PR will be shown.

### Quickstart with git flow
Tutorials for the beginners:

- https://guides.github.com/introduction/flow/index.html
- https://www.atlassian.com/git/tutorials/

Best practices:

- https://sethrobertson.github.io/GitBestPractices/
- http://nvie.com/posts/a-successful-git-branching-model/

### Travis build checks
Currently each PR is checked for proper build. Additionally it might be checked does PR:
- Compile without warnings (on at least one recent compiler version) (future)
- Pass llvm-format without changes (use a style config file if necessary) (future)
- Don't break ability to compile (at least on amd64, QT5)
We (would) use Travis service to enforce these and to always have binary builds.
