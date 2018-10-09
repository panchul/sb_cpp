//
// I think regexes are part of c++11, but here is how to use boos for that
//
//
// $ g++ -I $BOOST_ROOT ${BOOST_ROOT_LIB}/libboost_regex.a formatted_input_regex.cpp
//

#include <boost/regex.hpp>
#include <iostream>

int main()
{
   boost::regex re("\":(\\d+)\\+(\\d+)=(.+)\"");
   std::string example = "\":48754+7812=Abcs\"";
   boost::smatch match;
   if (boost::regex_match(example, match, re))
   {
      std::cout << "f number: " << match[1] << " s number: " << match[2] << " string: " << match[3]
      << std::endl;
   }
   else
   {
      std::cout << "not match" << std::endl;
   }
}
