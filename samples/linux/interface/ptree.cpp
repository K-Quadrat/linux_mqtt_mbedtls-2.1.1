#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <map>
#include <boost/foreach.hpp>



using namespace std;

using namespace boost::property_tree;
//char outputstring[100];


string writeJSON(boost::property_tree::ptree const& pt) {
    //convert property tree to JSON encoded string
    stringstream outputstring;
    outputstring << "{" << endl;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        outputstring << "\"" << it->first << "\":\"" << it->second.get_value<std::string>() << "\"";
        if (boost::next(it) != end) outputstring << "," << endl;
    }
    outputstring << endl << "}" << endl;
    return outputstring.str();
}


void printJSON(boost::property_tree::ptree const& pt) {

    //print property tree (JSON encoded Data)
    stringstream outputstring;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        outputstring << "\"" << it->first << "\":\"" << it-> second.get_value<std::string>() << "\"" << endl;
    }
    cout << outputstring.str();
}


ptree readJSON(string inputstring) {
    //convert JSON encoded inputstring to ptree
    ptree pt;

    //check for error before processing
//    if (errorcheck() != 0) return pt;

    try {
        //convert JSON to property tree
        std::istringstream is(inputstring);
        boost::property_tree::read_json(is, pt);
    }
    catch(boost::property_tree::json_parser::json_parser_error &je) {
        //catch exceptions
//        CLOG(ERROR, "JSON") << "Failed: readJSON: " << je.message();
//        pthread_mutex_lock(&errorMutex);
//        ::readJSONerror = -1;
//        pthread_mutex_unlock(&errorMutex);
        cout << "ERROR";
    }

    return pt;
}




int main()
{
//    ptree pt;
//    pt.put("channel1, channel2, channel3");


//    readJSON("Das ist ein Test");
//    printJSON(readJSON("{\"Test\":\"1234\"}"));

    ptree pt;
//    pt = readJSON("{\"channels\":[{\"name\":\"A\", \"subscribed\":true},{\"name\":\"B\", \"subscribed\":false},{\"name\":\"C\", \"subscribed\":true}]}");
    pt = readJSON("{\"channels\":[{\"name\":\"10\", \"subscribed\":true},{\"name\":\"11\", \"subscribed\":false},{\"name\":\"12\", \"subscribed\":true}]}");

//    cout << pt.get_child()<long>("name");

    char buffer[100];

    BOOST_FOREACH(const ptree::value_type &v, pt.get_child("channels")) {

                    if (v.second.get<string>("subscribed").compare("true") == 0){
                        cout << v.second.get<string>("name") << "\n";

                    }
                    else if (v.second.get<string>("subscribed").compare("false") == 0){
                        cout << v.second.get<string>("name") << "\n";

                    }


//        cout << v.second.get<string>("subscribed") << "\n";

        // v.first is the name of the child.
        // v.second is the child tree.
    }





//    printf("%s\n", writeJSON(pt));
//    std::cout << writeJSON(&pt);




/*    ptree pt2;
    json_parser::read_json("file.json", pt2);


    std::cout << std::boolalpha << (pt == pt2) << '\n';*/
}
