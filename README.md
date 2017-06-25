# LookBack Neuron Library

##  Installation

1.  Before installing, ensure that `nest-config` is in the path. Then run the
    following in the source directory

    ```
    cmake .
    cmake --build .
    ```

2.  The above should create a shell script called `enable_nest_lookback.sh`. This
    should be sourced to edit the relevant environment variables. (The line sourcing
    this can be added to the `.bashrc`)

##  How to use?

1.  The example provided can be referred to for this purpose.

2.  A point to note is that one will need to link to the lookback library. This can
    be acheived by adding a line as follows to the `CMakeLists.txt` (see the
    `CMakeLists.txt`)

    ```
    target_link_libraries(<targetname> lookbackmodule)
    ```

    Typically, in custom nest modules there are 2 targets `${MODULE_NAME}_module`
    and `${MODULE_NAME}_lib`. The above must be done for both.

##  Details regarding usage

<Coming Soon, until then the example and its readme should help>