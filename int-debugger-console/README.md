## Installation
Install all python requirements

``` $ pip3 install -r requirements.txt ```

## Usage 
Before starting the program, the file 'dips_testing.json' needs to be configured. 
There are two important options that need to be configured. 
The checkpointing-related functions `checkpoint_function` and `restore_function` needs to be configured to your own functions. 
By default it assumes that the functions are `checkpoint` and `restore`. 
The last important option is that `chip` needs to be configured to the right chip. 

The main entry point is `main.py`. You can enter the PUT as argument:

``` $ python main.py ../example-program/put.bin```

The next step is to connect to DIPS. This step also connects and halts the DUT. This can be done by running:

``` $ connect```


## Check for memory faults
Inside the file `dips_testing.json`, the `memcheck` fields need to be configured to your own project. The regions are the regions that are checked for memory differences. It is important to generate the folder `dumps` in order to store the memory dumps. 

To start the memory compare, run `dips_mem_check`

The check will automatically stop if a memory inconsistency is detected