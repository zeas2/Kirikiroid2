#ifndef tjsMT19937ar_cok_H
#define tjsMT19937ar_cok_H

/*
   A C-program for MT19937
   C++ wrapped version by W.Dee <dee@kikyou.info>
*/

namespace TJS
{


#define TJS_MT_N 624


struct tTJSMersenneTwisterData
{
	int left;
	unsigned long *next; /* points a value in 'state' */
	unsigned long state[TJS_MT_N]; /* the array for the state vector  */
};

class tTJSMersenneTwister : protected tTJSMersenneTwisterData
{

public:
	tTJSMersenneTwister(unsigned long s = 5489UL);
		/* initializes state[N] with a seed */
	tTJSMersenneTwister(unsigned long init_key[], unsigned long key_length);
		/* initialize by an array with array-length */
		/* init_key is the array for initializing keys */
		/* key_length is its length */
	tTJSMersenneTwister(const tTJSMersenneTwisterData &data);
		/* construct tTJSMersenneTwisterData data */

	virtual ~tTJSMersenneTwister() {;}

	static tTJSMersenneTwister& sharedInstance();

private:
	void init_genrand(unsigned long s);

	void next_state(void);

public:
	unsigned long int32(void); /* generates a random number on [0,0xffffffff]-interval */
	long int31(void); /* generates a random number on [0,0x7fffffff]-interval */
	double real1(void); /* generates a random number on [0,1]-real-interval */
	double real2(void); /* generates a random number on [0,1)-real-interval */
	double real3(void); /* generates a random number on (0,1)-real-interval */
	double res53(void); /* generates a random number on [0,1) with 53-bit resolution*/

	double rand_double(void); /* generates a random number on [0,1) with IEEE 64-bit double precision */

	const tTJSMersenneTwisterData & GetData() const { return *this; }
		/* retrieve data */
	void SetData(const tTJSMersenneTwisterData & rhs);
		/* set data */
};


} // namespace TJS

#endif
