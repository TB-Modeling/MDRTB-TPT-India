<!--Project-specific-->
# MDRTB ABS
An agent-based simulation for modeling multi-drug resistant tuberculosis in India.

![Picture1](https://user-images.githubusercontent.com/13045020/68499630-a432e380-0227-11ea-8229-33c2ccadf32b.png)
<!--/-->

# Foreword
The following documentation is will be largely applicable to anyone using any operating system. However, for Windows users, some of the commands (e.g. `ls`) will not work, and you'll need to use the Windows alternative.

# Getting Started
## Running simulations
<!--Documentation Metadata
  name: getting-started/running-the-model
  version: 1.0.0
-->
The project can be run locally. However, due to the complex and stochastic nature of the model, there are two problems with doing this: (i) the models takes a long time to run, between 5 minutes to an hour depending on which of our models you are running, and (ii) you want to run it hundreds or thousands of times in order to get statistically significant results.
 
Therefore, it is recommended to use a distributed approach, such as an HPC (High Performance Computing) cluster. If you are a member of JHU or UMD, [MARCC](https://www.marcc.jhu.edu/) is such a service that is available for free use. This is what the [modeltb.org](http://www.modeltb.org) team uses to run simulations.

<!--Project-specific-->
**Command line usage**  
`./mdrtb.out NODE_ID`

**Parameters**

| Param     | Type  | Example          | Description
|---        |---    |---               |---
| ```NODE_ID``` | ```int``` | ```./mdrtb.out 1``` | Used to determine HPC node instance. Required, albeit arbitrary if running locally.
<!--/-->

### Running locally
Download the latest executable binary program from <!--Project-specific-->[the releases page](https://github.com/TB-Modeling/mdrtb/releases)<!--/-->. The file name should end in `.out`. Then, run it as described in the section "command line usage" above.

### Running on MARCC
**1. Gaining access**  
If your project uses MARCC, you should be able to get an account. You can request access an account here: [Request an account](https://www.marcc.jhu.edu/request-access/request-an-account/). You'll receive several more emails to finish some other steps in the setup process.

**2. Logging in**  
Once you have access, log in to MARCC as described in [the documentation](https://www.marcc.jhu.edu/getting-started/connecting-to-marcc/).

**3. Running the simulations**  

*i. Set up working directory*  
When you're logged in, you'll see several directories available if you type [`ls`](http://man7.org/linux/man-pages/man1/ls.1.html). These standard directories are described in [the documentation](https://www.marcc.jhu.edu/getting-started/data-storage/).

You'll want to either (a) `cd scratch/`, "scratch" being the name of your perosnal, default private folder, or (b) create your own folder to work in and [`cd`](http://linuxcommand.org/lc3_man_pages/cdh.html) into that.

*ii. Clone the repository*  
Once inside the working directory, clone the project and change directory into its source files folder.

<!--Project-specific-->
`git clone https://github.com/TB-Modeling/mdrtb.git && cd mdrtb/src`
<!--/-->

*ii. Get the binary*  
The easiest way is to download the binary from <!--Project-specific-->[the releases page](https://github.com/TB-Modeling/mdrtb/releases)<!--/--> and [`scp`](https://linux.die.net/man/1/scp) it to MARCC. If you are a developer and building from source, you can also build it locally and then `scp` it, or just build it while on MARCC.

*iii. Set up environment*  
Most users will be using the SBATCH script (`src/sbatch.sh`), which automates this task, to submit a job to the MARCC HPC. If that describes or you're not sure, skip to the next step.

For those who are running the binary directly or doing something else custom on MARCC, there are certain modules that need to be made available in order for the simulations to run. While in the `src/` directory, type out `make marcc-load` and follow the directions that show on the screen. This should successfully set up the required modules for you. You can also read more about the MARCC module system in [its documentation](https://www.marcc.jhu.edu/getting-started/software/). 

*iv. Submit job to the cluster*  
While in the `src/` directory, type out `make run`. This will start a script that will submit a job to the HPC and run the simulation many times in paraellel.

### Understanding the output
This section has been moved to [the wiki](https://github.com/TB-Modeling/Modeling-Projects/wiki/Analyzing-results). 

### Analyzing the output
This section has been moved to [the wiki](https://github.com/TB-Modeling/Modeling-Projects/wiki/Analyzing-results). 

## Project setup and installation
<!--Documentation Metadata
  name: getting-started/project-setup-and-installation
  version: 1.0.0
-->
### 1. Download the project
<!--Project-specific-->
i. Fork the project
ii. Clone the project from your fork: `git clone https://github.com/YOUR_USERNAME/mdrtb.git`
<!--/-->

### 2. System requirements
Your system must meet the following requirements before you can begin installing the required C++ packages and build the executable.

*[Homebrew](http://brew.sh)* is the first thing that you should install if you don't already have it. Homebrew can then be used to install most of the other system requirements here.

Python is installed simply because it is the language that actually powers the C++ package manager *[Conan](https://conan.io/)*.

> **OS**: MacOS  
> **Language**: C++ 11 ([1](https://www.w3schools.in/cplusplus-tutorial/install/), [2](https://www.youtube.com/watch?v=1E_kBSka_ec))  
> **Language**: Python 3.x ([1](https://formulae.brew.sh/formula/python), [2](https://www.python.org/downloads/))  
> **Compiler**: GCC/G++ 6.5 ([1](https://formulae.brew.sh/formula/gcc))   
> **Version Control**: Git 2.x ([1](https://formulae.brew.sh/formula/git), [2](https://git-scm.com/book/en/v1/Getting-Started-Installing-Git))   
> **Package Manager for MacOS**: Homebrew 2.1+ ([1](https://brew.sh/))  
> **Package Manager for C++**: Conan 1.1+ ([1](https://github.com/conan-io/conan#from-homebrew-osx), [2](https://github.com/conan-io/conan#from-pip))

### 3. Installing C++ packages
*[Conan](https://conan.io/)* is a C++ package manager. The folowing files used by conan for installation.

> - **[conanfile.txt](https://docs.conan.io/en/latest/reference/conanfile_txt.html)**: Defines dependent packages & build system  
> - **[conanprofile.txt](https://docs.conan.io/en/latest/reference/profiles.html)**: Other settings required for installation

**i. Config updates**  
Open `~/.conan/settings.yml`

Navigate to the section that looks like this:
```yaml
    gcc:
        version: ["4.1", "4.4", "4.5", "4.6", "4.7", "4.8", "4.9",
                  "5", "5.1", "5.2", "5.3", "5.4", "5.5",
                  "6", "6.1", "6.2", "6.3", "6.4",
    ...
```

Add `" 6.5",` so that it looks like this:
```yaml
    gcc:
        version: ["4.1", "4.4", "4.5", "4.6", "4.7", "4.8", "4.9",
                  "5", "5.1", "5.2", "5.3", "5.4", "5.5",
                  "6", "6.1", "6.2", "6.3", "6.4", "6.5",
    ...
```

Save and close the file.

**ii. Running installation**  
With Conan installed on your system, the installation can be performed  by running [`conan install`](https://docs.conan.io/en/latest/reference/commands/consumer/install.html). Some modifications have been made to make for a smoother installation on MacOS, so for Macs, `cd` into the `src/` directory and install via:

`make install`

For Windows, Linux or other environments, please submit an issue or send an email to jflack@jhu.edu for support. But the gist of it is that it will involve a variation of the command in the makefile that uses the profile in `~/.conan/profiles/default`, which also needs to be updated a little prior to installation.

## Building the model
- [Building the model](https://github.com/TB-Modeling/Modeling-Projects/wiki/Building-the-model)

# Xcode users
- [Setting up Xcode](https://github.com/TB-Modeling/Modeling-Projects/wiki/Setting-up-Xcode)

# Advanced topics
- [Codebase overview](https://github.com/TB-Modeling/Modeling-Projects/wiki/Codebase-overview)
- [Coding conventions](https://github.com/TB-Modeling/Modeling-Projects/wiki/Coding-conventions)
- [Updating dependencies](https://github.com/TB-Modeling/Modeling-Projects/wiki/Updating-dependencies)
- [Running automated tests](https://github.com/TB-Modeling/Modeling-Projects/wiki/Running-automated-tests)
