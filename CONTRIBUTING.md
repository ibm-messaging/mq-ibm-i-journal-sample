# Contributing to mq-ibm-i-journal-sample

Thank you for your interest in contributing to the open-source project, Automating Management of IBM MQ Journal Receivers on IBM i.

To ensure that the codebase is always healthy and does not result in deployment issues when forked and used, it is important that you pre-check your additions and updates for any potential code conflicts before uploading your changes to the GitHub Repository. 

Therefore, the following steps should be followed to submit your contributions: 

1. Fork the repository
2. Run test, format
3. Commit/Push changes to your fork
4. Create a Pull Request 


### 1. Fork the repository

To fork the repository:
- Get started by clicking on "Fork" from the top-right corner of the main repository page.
- Choose a name and description for your fork.
- Select the option "Copy the main branch only", as in most cases, you will only need the default branch to be copied.
- Click on "Create fork".

Once you have forked the repository, you can then clone your fork to your computer locally. In order to do that:
- Click on "Code" (the green button on your forked repository).
- Copy the forked repository URL under HTTPS.
- Type the following on your terminal:

```
git clone <the_forked_repository_url> 
cd mq-ibm-i-journal-sample
```

You can set up Git to pull updates from the repository into the local clone of your fork when you fork a project in order to propose changes to the repository. In order to do that, run the following command:

```
git remote add upstream https://github.com/ibm-messaging/mq-ibm-i-journal-sample/
```

To verify the new upstream repository you have specified for your fork, run the following command:

```
git remote -v
```

You should see the URL for your fork as origin, and the URL for our repository as upstream.

Now, you can work locally and commit to your changes to your fork. This will not impact the main branch.

### 2. Run tests and format script

Before committing changes, build on IBM i and run manual tests to ensure that they work with the main codebase. 

### 3. Commit/Push changes to your fork 

If you are looking to add all the files you have modified in a particular directory, you can stage them all with the following command:

```
git add . 
```

If you are looking to recursively add all changes including those in subdirectories, you can type: 

```
git add -A 
```

Alternatively, you can type _git add -all_ for all new files to be staged. 

Make sure that no binaries that get built are included in the pull request.

Once you are ready to submit your changes, ensure that you commit them to your fork with a message. The commit message is an important aspect of your code contribution; it helps the maintainers and other contributors to fully understand the change you have made, why you made it, and how significant it is. 

You can commit your changes by running: 

```
git commit -s -m "Brief description of your changes/additions"
```

All commits must be signed.

To push all your changes to the forked repo:

```
git push
```

### 4. Create a Pull Request

Merge any changes that were made in the original repository’s main branch:

```
git merge upstream/main
```

Before creating a Pull Request, ensure you have read the [IBM Contributor License Agreement](CLA.md). By creating a PR, you certify that your contribution:
1. is licensed under Apache Licence Version 2.0, The MIT License, or any BSD License.

Once you have carefully read and agreed to the terms mentioned in the [CLA](CLA.md), you are ready to make a pull request to the original repository.

Navigate to your forked repository and press the _New pull request_ button. Then, you should add a title and a comment to the appropriate fields and then press the _Create pull request_ button.

The maintainers of our repository will then review your contribution and decide whether or not to accept your pull request.
 
