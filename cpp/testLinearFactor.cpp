/**
 *  @file   testLinearFactor.cpp
 *  @brief  Unit tests for Linear Factor
 *  @author Christian Potthast
 *  @author Frank Dellaert
 **/

#include <iostream>

#include <boost/tuple/tuple.hpp>
#include <boost/assign/std/list.hpp> // for operator +=
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/map.hpp> // for insert
using namespace boost::assign;

#include <CppUnitLite/TestHarness.h>

#include "Matrix.h"
#include "Ordering.h"
#include "ConditionalGaussian.h"
#include "smallExample.h"

using namespace std;
using namespace gtsam;

/* ************************************************************************* */
TEST( LinearFactor, linearFactor )
{
	double sigma = 0.1;

	Matrix A1(2,2);
	A1(0,0) = -1.0 ; A1(0,1) = 0.0;
	A1(1,0) = 0.0 ; A1(1,1) = -1.0;

	Matrix A2(2,2);
	A2(0,0) = 1.0 ; A2(0,1) = 0.0;
	A2(1,0) = 0.0 ; A2(1,1) = 1.0;

	Vector b(2);
	b(0) = 0.2 ; b(1) = -0.1;

	LinearFactor expected("x1", A1,  "x2", A2, b, sigma);

	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the factor "f2" from the factor graph
	LinearFactor::shared_ptr lf = fg[1];

	// check if the two factors are the same
	CHECK(assert_equal(expected,*lf));
}

/* ************************************************************************* */
TEST( LinearFactor, keys )
{
	// get the factor "f2" from the small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();
	LinearFactor::shared_ptr lf = fg[1];
	list<string> expected;
	expected.push_back("x1");
	expected.push_back("x2");
	CHECK(lf->keys() == expected);
}

/* ************************************************************************* */
TEST( LinearFactor, dimensions )
{	
  // get the factor "f2" from the small linear factor graph
  LinearFactorGraph fg = createLinearFactorGraph();

  // Check a single factor
  Dimensions expected;
  insert(expected)("x1", 2)("x2", 2);
  Dimensions actual = fg[1]->dimensions();
  CHECK(expected==actual);
}

/* ************************************************************************* */
TEST( LinearFactor, getDim )
{
	// get a factor
	LinearFactorGraph fg = createLinearFactorGraph();
	LinearFactor::shared_ptr factor = fg[0];

	// get the size of a variable
	size_t actual = factor->getDim("x1");

	// verify
	size_t expected = 2;
	CHECK(actual == expected);
}

/* ************************************************************************* */
TEST( LinearFactor, combine )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get two factors from it and insert the factors into a vector
	vector<LinearFactor::shared_ptr> lfg;
	lfg.push_back(fg[4 - 1]);
	lfg.push_back(fg[2 - 1]);

	// combine in a factor
	LinearFactor combined(lfg);

	// sigmas
	double sigma2 = 0.1;
	double sigma4 = 0.2;
	Vector sigmas = Vector_(4, sigma4, sigma4, sigma2, sigma2);

	// the expected combined linear factor
	Matrix Ax2 = Matrix_(4, 2, // x2
			-1., 0.,
			+0., -1.,
			1., 0.,
			+0., 1.);

	Matrix Al1 = Matrix_(4, 2,	// l1
			1., 0.,
			0., 1.,
			0., 0.,
			0., 0.);

	Matrix Ax1 = Matrix_(4, 2,	// x1
			0.00, 0., // f4
			0.00, 0., // f4
			-1., 0., // f2
			0.00, -1. // f2
	);

	// the RHS
	Vector b2(4);
	b2(0) = -0.2;
	b2(1) = 0.3;
	b2(2) = 0.2;
	b2(3) = -0.1;

	// use general constructor for making arbitrary factors
	vector<pair<string, Matrix> > meas;
	meas.push_back(make_pair("x2", Ax2));
	meas.push_back(make_pair("l1", Al1));
	meas.push_back(make_pair("x1", Ax1));
	LinearFactor expected(meas, b2, sigmas);
	CHECK(assert_equal(expected,combined));
}

/* ************************************************************************* */
TEST( NonlinearFactorGraph, combine2){
	double sigma1 = 0.0957;
	Matrix A11(2,2);
	A11(0,0) = 1; A11(0,1) =  0;
	A11(1,0) = 0;       A11(1,1) = 1;
	Vector b(2);
	b(0) = 2; b(1) = -1;
	LinearFactor::shared_ptr f1(new LinearFactor("x1", A11, b*sigma1, sigma1));

	double sigma2 = 0.5;
	A11(0,0) = 1; A11(0,1) =  0;
	A11(1,0) = 0; A11(1,1) = -1;
	b(0) = 4 ; b(1) = -5;
	LinearFactor::shared_ptr f2(new LinearFactor("x1", A11, b*sigma2, sigma2));

	double sigma3 = 0.25;
	A11(0,0) = 1; A11(0,1) =  0;
	A11(1,0) = 0; A11(1,1) = -1;
	b(0) = 3 ; b(1) = -88;
	LinearFactor::shared_ptr f3(new LinearFactor("x1", A11, b*sigma3, sigma3));

	// TODO: find a real sigma value for this example
	double sigma4 = 0.1;
	A11(0,0) = 6; A11(0,1) =  0;
	A11(1,0) = 0; A11(1,1) = 7;
	b(0) = 5 ; b(1) = -6;
	LinearFactor::shared_ptr f4(new LinearFactor("x1", A11*sigma4, b*sigma4, sigma4));

	vector<LinearFactor::shared_ptr> lfg;
	lfg.push_back(f1);
	lfg.push_back(f2);
	lfg.push_back(f3);
	lfg.push_back(f4);
	LinearFactor combined(lfg);

	Vector sigmas = Vector_(8, sigma1, sigma1, sigma2, sigma2, sigma3, sigma3, sigma4, sigma4);
	Matrix A22(8,2);
	A22(0,0) = 1; A22(0,1) =  0;
	A22(1,0) = 0;       A22(1,1) = 1;
	A22(2,0) = 1;       A22(2,1) =  0;
	A22(3,0) = 0;       A22(3,1) = -1;
	A22(4,0) = 1;       A22(4,1) =  0;
	A22(5,0) = 0;       A22(5,1) = -1;
	A22(6,0) = 0.6;       A22(6,1) =  0;
	A22(7,0) = 0;       A22(7,1) =  0.7;
	Vector exb(8);
	exb(0) = 2*sigma1 ; exb(1) = -1*sigma1;  exb(2) = 4*sigma2 ; exb(3) = -5*sigma2;
	exb(4) = 3*sigma3 ; exb(5) = -88*sigma3; exb(6) = 5*sigma4 ; exb(7) = -6*sigma4;

	vector<pair<string, Matrix> > meas;
	meas.push_back(make_pair("x1", A22));
	LinearFactor expected(meas, exb, sigmas);
	CHECK(assert_equal(expected,combined));
}

/* ************************************************************************* */
TEST( LinearFactor, linearFactorN){
  vector<LinearFactor::shared_ptr> f;
  f.push_back(LinearFactor::shared_ptr(new LinearFactor("x1", Matrix_(2,2,
      1.0, 0.0,
      0.0, 1.0),
      Vector_(2,
      10.0, 5.0), 1)));
  f.push_back(LinearFactor::shared_ptr(new LinearFactor("x1", Matrix_(2,2,
      -10.0, 0.0,
      0.0, -10.0),
      "x2", Matrix_(2,2,
      10.0, 0.0,
      0.0, 10.0),
      Vector_(2,
      1.0, -2.0), 1)));
  f.push_back(LinearFactor::shared_ptr(new LinearFactor("x2", Matrix_(2,2,
      -10.0, 0.0,
      0.0, -10.0),
      "x3", Matrix_(2,2,
      10.0, 0.0,
      0.0, 10.0),
      Vector_(2,
      1.5, -1.5), 1)));
  f.push_back(LinearFactor::shared_ptr(new LinearFactor("x3", Matrix_(2,2,
      -10.0, 0.0,
      0.0, -10.0),
      "x4", Matrix_(2,2,
      10.0, 0.0,
      0.0, 10.0),
      Vector_(2,
      2.0, -1.0), 1)));

  LinearFactor combinedFactor(f);

  vector<pair<string, Matrix> > combinedMeasurement;
  combinedMeasurement.push_back(make_pair("x1", Matrix_(8,2,
      1.0, 0.0,
      0.0, 1.0,
      -10.0, 0.0,
      0.0, -10.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0)));
  combinedMeasurement.push_back(make_pair("x2", Matrix_(8,2,
      0.0, 0.0,
      0.0, 0.0,
      10.0, 0.0,
      0.0, 10.0,
      -10.0, 0.0,
      0.0, -10.0,
      0.0, 0.0,
      0.0, 0.0)));
  combinedMeasurement.push_back(make_pair("x3", Matrix_(8,2,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      10.0, 0.0,
      0.0, 10.0,
      -10.0, 0.0,
      0.0, -10.0)));
  combinedMeasurement.push_back(make_pair("x4", Matrix_(8,2,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      0.0, 0.0,
      10.0, 0.0,
      0.0, 10.0)));
  Vector b = Vector_(8,
      10.0, 5.0, 1.0, -2.0, 1.5, -1.5, 2.0, -1.0);

  LinearFactor expected(combinedMeasurement, b, 1.);
  CHECK(combinedFactor.equals(expected));
}

/* ************************************************************************* */
TEST( LinearFactor, error )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the first factor from the factor graph
	LinearFactor::shared_ptr lf = fg[0];

	// check the error of the first factor with noisy config
	VectorConfig cfg = createZeroDelta();

	// calculate the error from the factor "f1"
	// note the error is the same as in testNonlinearFactor
	double actual = lf->error(cfg);
	DOUBLES_EQUAL( 1.0, actual, 0.00000001 );
}

/* ************************************************************************* */
TEST( LinearFactor, eliminate )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get two factors from it and insert the factors into a vector
	vector<LinearFactor::shared_ptr> lfg;
	lfg.push_back(fg[4 - 1]);
	lfg.push_back(fg[2 - 1]);

	// combine in a factor
	LinearFactor combined(lfg);

	// eliminate the combined factor
	ConditionalGaussian::shared_ptr actualCG;
	LinearFactor::shared_ptr actualLF;
	boost::tie(actualCG,actualLF) = combined.eliminate("x2");

	// create expected Conditional Gaussian
	Matrix R11 = Matrix_(2,2,
			1.0, 0.0,
			0.0, 1.0
	);
	Matrix S12 = Matrix_(2,2,
			-0.2, 0.0,
			+0.0,-0.2
	);
	Matrix S13 = Matrix_(2,2,
			-0.8, 0.0,
			+0.0,-0.8
	);
	Vector d(2); d(0) = 0.2; d(1) = -0.14;

	Vector sigmas(2);
	sigmas(0) = 1/sqrt(125.0);
	sigmas(1) = 1/sqrt(125.0);

	// Check the conditional Gaussian
	ConditionalGaussian expectedCG("x2", d,R11,"l1",S12,"x1",S13,sigmas);

	// the expected linear factor
	double sigma = 0.2236;
	Matrix Bl1 = Matrix_(2,2,
			// l1
			1.00, 0.00,
			0.00, 1.00
	);

	Matrix Bx1 = Matrix_(2,2,
			// x1
			-1.00,  0.00,
			+0.00, -1.00
	);

	// the RHS
	Vector b1(2); b1(0) = 0.0; b1(1) = 0.2;

	LinearFactor expectedLF("l1", Bl1, "x1", Bx1, b1, sigma);

	// check if the result matches
	CHECK(assert_equal(expectedCG,*actualCG,1e-4));
	CHECK(assert_equal(expectedLF,*actualLF,1e-5));
}


/* ************************************************************************* */
TEST( LinearFactor, eliminate2 )
{
	// sigmas
	double sigma1 = 0.2;
	double sigma2 = 0.1;
	Vector sigmas = Vector_(4, sigma1, sigma1, sigma2, sigma2);

	// the combined linear factor
	Matrix Ax2 = Matrix_(4,2,
			// x2
			-1., 0.,
			+0.,-1.,
			1., 0.,
			+0.,1.
	);

	Matrix Al1x1 = Matrix_(4,4,
			// l1   x1
			1., 0., 0.00,  0., // f4
			0., 1., 0.00,  0., // f4
			0., 0., -1.,  0., // f2
			0., 0., 0.00,-1.  // f2
	);

	// the RHS
	Vector b2(4);
	b2(0) = -0.2;
	b2(1) =  0.3;
	b2(2) =  0.2;
	b2(3) = -0.1;

	vector<pair<string, Matrix> > meas;
	meas.push_back(make_pair("x2", Ax2));
	meas.push_back(make_pair("l1x1", Al1x1));
	LinearFactor combined(meas, b2, sigmas);

	// eliminate the combined factor
	ConditionalGaussian::shared_ptr actualCG;
	LinearFactor::shared_ptr actualLF;
	boost::tie(actualCG,actualLF) = combined.eliminate("x2");

	// create expected Conditional Gaussian
	Matrix R11 = Matrix_(2,2,
			1.00,  0.00,
			0.00,  1.00
	);
	Matrix S12 = Matrix_(2,4,
			-0.20, 0.00,-0.80, 0.00,
			+0.00,-0.20,+0.00,-0.80
	);
	Vector d(2); d(0) = 0.2; d(1) = -0.14;

	Vector x2Sigmas(2);
	x2Sigmas(0) = 0.0894427;
	x2Sigmas(1) = 0.0894427;

	ConditionalGaussian expectedCG("x2",d,R11,"l1x1",S12,x2Sigmas);

	// the expected linear factor
	double sigma = 0.2236;
	Matrix Bl1x1 = Matrix_(2,4,
			// l1          x1
			1.00, 0.00, -1.00,  0.00,
			0.00, 1.00, +0.00, -1.00
	);

	// the RHS
	Vector b1(2); b1(0) = 0.0; b1(1) = 0.894427;

	LinearFactor expectedLF("l1x1", Bl1x1, b1*sigma, sigma);

	// check if the result matches
	CHECK(assert_equal(expectedCG,*actualCG,1e-4));
	CHECK(assert_equal(expectedLF,*actualLF,1e-5));
}

/* ************************************************************************* */
TEST( LinearFactor, default_error )
{
	LinearFactor f;
	VectorConfig c;
	double actual = f.error(c);
	CHECK(actual==0.0);
}

//* ************************************************************************* */
TEST( LinearFactor, eliminate_empty )
{
	// create an empty factor
	LinearFactor f;

	// eliminate the empty factor
	ConditionalGaussian::shared_ptr actualCG;
	LinearFactor::shared_ptr actualLF;
	boost::tie(actualCG,actualLF) = f.eliminate("x2");

	// expected Conditional Gaussian is just a parent-less node with P(x)=1
	ConditionalGaussian expectedCG("x2");

	// expected remaining factor is still empty :-)
	LinearFactor expectedLF;

	// check if the result matches
	CHECK(actualCG->equals(expectedCG));
	CHECK(actualLF->equals(expectedLF));
}

//* ************************************************************************* */
TEST( LinearFactor, empty )
{
	// create an empty factor
	LinearFactor f;
	CHECK(f.empty()==true);
}

/* ************************************************************************* */
TEST( LinearFactor, matrix )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the factor "f2" from the factor graph
	LinearFactor::shared_ptr lf = fg[1];

	// render with a given ordering
	Ordering ord;
	ord += "x1","x2";

	Matrix A; Vector b;
	boost::tie(A,b) = lf->matrix(ord);

	Matrix A1 = Matrix_(2,4,
			-10.0,  0.0, 10.0,  0.0,
			000.0,-10.0,  0.0, 10.0 );
	Vector b1 = Vector_(2, 2.0, -1.0);

	EQUALITY(A,A1);
	EQUALITY(b,b1);
}

/* ************************************************************************* */
TEST( LinearFactor, matrix_aug )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the factor "f2" from the factor graph
	LinearFactor::shared_ptr lf = fg[1];

	// render with a given ordering
	Ordering ord;
	ord += "x1","x2";

	Matrix Ab;
	Ab = lf->matrix_augmented(ord);

	Matrix Ab1 = Matrix_(2,5,
			-10.0,  0.0, 10.0,  0.0, 2.0,
			000.0,-10.0,  0.0, 10.0, -1.0 );

	EQUALITY(Ab,Ab1);
}

/* ************************************************************************* */
// small aux. function to print out lists of anything
template<class T>
void print(const list<T>& i) {
	copy(i.begin(), i.end(), ostream_iterator<T> (cout, ","));
	cout << endl;
}

/* ************************************************************************* */
TEST( LinearFactor, sparse )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the factor "f2" from the factor graph
	LinearFactor::shared_ptr lf = fg[1];

	// render with a given ordering
	Ordering ord;
	ord += "x1","x2";

	list<int> i,j;
	list<double> s;
	boost::tie(i,j,s) = lf->sparse(ord, fg.dimensions());

	list<int> i1,j1;
	i1 += 1,2,1,2;
	j1 += 1,2,3,4;

	list<double> s1;
	s1 += -10,-10,10,10;

	CHECK(i==i1);
	CHECK(j==j1);
	CHECK(s==s1);
}

/* ************************************************************************* */
TEST( LinearFactor, sparse2 )
{
	// create a small linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get the factor "f2" from the factor graph
	LinearFactor::shared_ptr lf = fg[1];

	// render with a given ordering
	Ordering ord;
	ord += "x2","l1","x1";

	list<int> i,j;
	list<double> s;
	boost::tie(i,j,s) = lf->sparse(ord, fg.dimensions());

	list<int> i1,j1;
	i1 += 1,2,1,2;
	j1 += 1,2,5,6;

	list<double> s1;
	s1 += 10,10,-10,-10;

	CHECK(i==i1);
	CHECK(j==j1);
	CHECK(s==s1);
}

/* ************************************************************************* */
TEST( LinearFactor, size )
{
	// create a linear factor graph
	LinearFactorGraph fg = createLinearFactorGraph();

	// get some factors from the graph
	boost::shared_ptr<LinearFactor> factor1 = fg[0];
	boost::shared_ptr<LinearFactor> factor2 = fg[1];
	boost::shared_ptr<LinearFactor> factor3 = fg[2];

	CHECK(factor1->size() == 1);
	CHECK(factor2->size() == 2);
	CHECK(factor3->size() == 2);
}

/* ************************************************************************* */
TEST( LinearFactor, CONSTRUCTOR_ConditionalGaussian )
{
	Matrix R11 = Matrix_(2,2,
			1.00,  0.00,
			0.00,  1.00
	);
	Matrix S12 = Matrix_(2,2,
			-0.200001, 0.00,
			+0.00,-0.200001
	);
	Vector d(2); d(0) = 2.23607; d(1) = -1.56525;

	Vector sigmas(2);
	sigmas(0) = 0.29907;
	sigmas(1) = 0.29907;

	ConditionalGaussian::shared_ptr CG(new ConditionalGaussian("x2",d,R11,"l1x1",S12,sigmas));
	LinearFactor actualLF(CG);
	//  actualLF.print();
	LinearFactor expectedLF("x2",R11,"l1x1",S12,d, sigmas(0));

	CHECK(assert_equal(expectedLF,actualLF,1e-5));
}
/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr);}
/* ************************************************************************* */
