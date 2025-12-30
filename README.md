# simulation-engine

**Requirements**

* Apache arrow - make sure you install the C++ version, not the Python version
* ninja
* cmake 4.0.0 or later

**Compilation**

To build the simulation engine, make sure you do not have a previous build. If you do delete it with ```rm -rf build```.
Then compile with ```cmake -B build -G Ninja && cmake --build build```
Running the examples There are two examples for placing market orders and limit orders. They can each be run by executing them as such. After compiling they will be located in the build directory.:

* ```./build/market_order_example.cpp```
* ```./build/limit_order_example.cpp```

1. Creating and testing your own strategy
2. Create a directory named ```strategies/``` inside simulation_engine.
3. Create a cpp file my_strategy_name.cpp inside this new directory.
4. Create a class that inherits from ```IStrategy<N>```, where N is the number of price levels you want your strategy to see. For ten levels of the book, inherit from ```Istrategy<10>```.
5. In this class create public constructor and ```onMarketData(const Quote<N>& quote) ovverride``` methods. Then place your logic for how the strategy should deal with incoming quotes in this onMarketData method.
6. Add the file to the ```CMakeLists.txt``` file.
7. Compile and run

For an example of what this should look like, look at the ```examples/limit_order_example.cpp``` and ```examples/market_order_example.cpp files```.