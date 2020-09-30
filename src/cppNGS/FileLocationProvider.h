#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H

class FileLocationProvider
{
public:
	static void create(int,int);

	static FileLocationProvider* getInstance();
	static bool exists();

	inline int getVariants(){ return variants; };
	inline void setVariants(int _in){ variants = _in; };

	inline int getFilename(){ return filename; };
	inline void setFilename(int _in){ filename = _in; };

//	static void doSomething();
//	static void doSomethingElse();
protected:
	FileLocationProvider(int, int);
	virtual ~FileLocationProvider(){};
	static FileLocationProvider* theOnlyInstance;
private:
	int variants;
	int filename;
};




#endif // FILELOCATIONPROVIDER_H
