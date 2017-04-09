

// Compiled this on Mac like so:
// g++ -I $BOOST_ROOT boost_lambda1.cpp
// should work ok on Linux too.
//

#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>

int main()
{
    using namespace boost::lambda;
    typedef std::istream_iterator<int> in;
    // for running like "echo 1 2 3 | ./example", should print 3 6 9
    std::for_each(
        in(std::cin), in(), std::cout << (_1 * 3) << " " );
}
