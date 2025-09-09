# Coding convention

## Coding

__Classes/methods/variables__  
Class names are written in camel-case with the first letter upper-case.  
Method names are written in camel-case with the first letter lower-case.  
Variable names are written in lower-case with underscores sparating words. Private/protected member variables are suffixed with an underscore.  
Constants/Enums are be written in upper-case  with underscores sparating words.  

__Brackets__  
Brackets are always aligned vertically.  
If-statements are written with brackets, or in one line.

__Getters/setters__  
Getter methods should be named just like the property they return, but not prefixed with `get`.  
Setter methods should be prefixed with `set`.

__Example:__

    class MyClass
    {
    public:
    	enum SomeType
    	{
    		FIRST_POSSIBLE_VALUE,
    		SECOND_POSSIBLE_VALUE
    	};
    	
    	MyClass()
    		: some_value_(FIRST_POSSIBLE_VALUE)
    	{
    	}
    	
    	void setSomeValue(SomeType some_value)
    	{
    		some_value_ = some_value;
    	}
    	
    	SomeType someValue() const
    	{
    		return some_value;
    	}
    
    protected:
    	SomeType some_value_;
    };

## Testing

Code testing is important to make sure the code really does what you intended it to do. Additionally, a good test coverage
allows large-scale refactoring without worring that you have broken the functionality.  
Each class method with complicated code or more then three lines of code must be tested in a unit test.

## Documenting

Each class and method should be documented. The documentation is done in the source code 
using [Doxygen](http://www.stack.nl/~dimitri/doxygen/).  

## Optimization

Premature optimization is the cause of many bugs.  
Initially, a straight-forward version of each algorithm should be implemented, tested thoroughly and benchmarked.  
After that, a more optimized version of the algorithm can be implemented.  
Profiling the code before the optimizing is crucial, unless you are not 100% sure where the bottleneck is. A very easy-to-use profiler is [VerySleepy](http://www.codersnotes.com/sleepy).


[Back to main page](index.md)
