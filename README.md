# LookBack Neuron Library

##  Installation

1.  Before installing, ensure that `nest-config` is in the path. Then run the
    following from the source directory

    ```
    ./install.sh
    ```

##  How to use?

1.  The example provided can be referred to for this purpose.

2.  A point to note is that one will need to link to the lookback library when
    using it in some other module. This can be acheived by adding a line as
    follows to the `CMakeLists.txt` of the module (see the `CMakeLists.txt`).

    ```
    target_link_libraries(<targetname> lookbackmodule)
    ```

    Typically, in custom nest modules there are 2 targets
    `${MODULE_NAME}_module` and `${MODULE_NAME}_lib`. The above must be done
    for both.

##  Details regarding usage

<Coming Soon, until then the example and its readme should help>