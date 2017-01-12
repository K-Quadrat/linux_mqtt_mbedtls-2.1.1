#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <map>


using namespace std;

using namespace boost::property_tree;
//char outputstring[100];


string writeJSON(boost::property_tree::ptree const& pt) {
    //convert property tree to JSON encoded string
    stringstream outputstring;
    outputstring << "{" << endl;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        outputstring << "\"" << it->first << "\":\"" << it-> second.get_value<std::string>() << "\"";
        if (boost::next(it) != end) outputstring << "," << endl;
    }
    outputstring << endl << "}" << endl;
    return outputstring.str();
}



int main()
{
    ptree pt;
    pt.put("channel1, channel2, channel3");

    char buffer[1000];



//    printf("%s\n", writeJSON(pt));
    std::cout << writeJSON(&pt);




/*    ptree pt2;
    json_parser::read_json("file.json", pt2);


    std::cout << std::boolalpha << (pt == pt2) << '\n';*/
}
