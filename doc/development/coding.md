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

Code testing is important to make sure the code really does what you intended it to do.  
Additionally, a good test coverage allows large-scale refactoring without worring that you have broken the functionality.  
Each class method with complicated code or more then three lines of code must be tested in a unit test.  

See the documenation of the [test framewok](https://github.com/marc-sturm/cppTFW/) for details.

## Documenting

Each class and method should be documented. The documentation is done in the source code.

## Optimization

Donald Knuth coined the phrase `premature optimization is the root of all evil`.  
What it means is that most code does not need to be optimized. Optimize only code that is crucial for the runtime.  
Why? Because optimized code is often more complex and harder to maintain than simple code.

Initially, a straight-forward version of each algorithm should be implemented, tested thoroughly and benchmarked.  
After that, a more optimized version of the algorithm can be implemented of that is necessary.  
Profiling the code before the optimizing is crucial, unless you are a 100% sure where the bottleneck is.

[Back to main page](index.md)
