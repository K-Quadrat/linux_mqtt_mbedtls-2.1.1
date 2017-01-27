#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <map>
#include <boost/foreach.hpp>

#include <vector>




using namespace std;

using namespace boost::property_tree;
//char outputstring[100];

string mapToJSON(map<string, string> myMap) {

    ostringstream json;

    json.str("");
    json << "{";

    bool first = true;

    for (auto itmap=myMap.begin();itmap!=myMap.end();++itmap) {

        //filter out table specific ids
        if (!itmap->first.compare("binaryRun_id")) continue;
        if (!itmap->first.compare("jobRun_id")) continue;
        if (!itmap->first.compare("result_id")) continue;

        if (!first) json << ",";
        first = false;

        json << "\"" << itmap->first << "\":";

        CTool::stringReplace(itmap->second, "\n", "\\n");
        CTool::stringReplace(itmap->second, "\r", "\\r");
        CTool::stringReplace(itmap->second, "\b", "\\b");
        CTool::stringReplace(itmap->second, "\t", "\\t");
        CTool::stringReplace(itmap->second, "\f", "\\f");

        json << "\"" << itmap->second << "\"";
    }

    json << "}";

    return json.str();
}



int main()
{
    return 0;
}
