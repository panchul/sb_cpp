//
// $ g++ -std=c++1z boost_lambda.cpp -lpthread -I $(BOOST_ROOT) 
// or
// $ clang++ boost_lambda.cpp
//

#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>

int main()
{
    using namespace boost::lambda;
    typedef std::istream_iterator<int> in;

    std::for_each(
        in(std::cin), in(), std::cout << (_1 * 3) << " " );
}
