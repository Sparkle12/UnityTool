#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <cctype>
#include <map>
#include <deque>
#include <algorithm>
#include <set>
#include <chrono>
#include <thread>
namespace fs = std::filesystem;


struct object
{
    std::string id;
    std::string transform_id;
    std::string name;
    std::vector<std::string> children;
    std::string father;
};

void get_script_meta_paths(std::vector<std::string> &paths,const std::string &dir_path)
{   
    // The function goes through every file in the /Assets/Scripts/ directory and adds the name of the .cs.meta files to paths
    // If the function encounters a directory (can't find a '.' in the name) it dose a recursive call with that directory as the dir_path
     for(const auto& entry : fs::directory_iterator(dir_path))
        {   
            std::string temp = entry.path();
            std::istringstream iss(temp);
            std::string token;

            while(std::getline(iss,token,'/'));

            if(token.find(std::string(".cs.meta")) != std::string::npos)
                paths.emplace_back(entry.path());
            else
                if(token.find(std::string(".")) == std::string::npos)
                    get_script_meta_paths(paths,entry.path());
        }
}

void get_scene_paths(std::vector<std::string> &paths,const std::string &dir_path)
{
    // The function goes to every file in the /Assets/Scenes/ directory and adds the name of the .unity scene files to paths
    // If the function encounters a directory it dose a recursive call with that directory as the dir_path
    for (const auto & entry : fs::directory_iterator(dir_path))
            {
                std::string temp = entry.path();
                std::istringstream iss(temp);
                std::string token;
                while( std::getline(iss,token,'/'));
               
                if(token.find(std::string(".meta"))  == std::string::npos)
                    {
                        paths.emplace_back(token);
                    }
                
                if(token.find(std::string(".")) == std::string::npos)
                    get_scene_paths(paths,entry.path());

            }
    
}

void get_script_guid(std::set<std::string> &all_scripts, 
                    std::map<std::string,std::string> &script_guid_to_path, 
                    const std::vector<std::string> script_paths,
                    const char* dir_path)
{
    // The function parses every .cs.meta file and extracts the guid and relative path of each scripts
    // all_scripts is a set of all the script guids
    // scripts_guid_to_path is a map that outputs the relative path of the script using its guid 

    for(auto p : script_paths)
            {
                std::fstream in(p);
                std::string content;

                while(in>>content)
                    if(content == "guid:")
                        {
                            in>>content;
                            std::string relative_path = p;
                            relative_path.erase(0,strlen(dir_path) + 1);
                            relative_path.erase(relative_path.length()-5,5);
                            script_guid_to_path[content] = relative_path;
                            all_scripts.emplace(content);
                            break;
                        }
            }
}

void output_scene_hierarchy(std::vector<std::string> roots,
                            std::map<std::string,object> scene_objects,
                            const std::string scene_path,
                            const char* output_dir)
{
    // The deque hold pairs of (transform_id, depth) type
    // An object found in the scene roots section has a depth of 0
    // A child of an object has depth = father.depth + 1
    std::deque<std::pair<std::string, int>> deq;

    // Push all the scene root objects to the deque with a depth of 0
    for(auto root : roots)
        deq.push_back(std::pair(root,0));

    // Open the filestream to the output directory and create the .dump file
    std::string output_path = output_dir;
    output_path.append("/");
    std::ofstream out;
    out.open(output_path.append(scene_path).append(".dump"));


    while(!deq.empty())
    {
        auto curr = deq.front();
        deq.pop_front();

        // Print the objects name with appropriate depth
        for(int i = 0;i<curr.second;i++)
            out<<"--";
                        
        out<<scene_objects[curr.first].name<<std::endl;

        // If the object has any children push them to the front of the deque with depth + 1
        /* The children of an object are stored in reverse order in memory, 
        so when we push them to the front of the deque they will be in the right order*/

        if(scene_objects[curr.first].children.size() != 0)
        {
            for(auto child : scene_objects[curr.first].children)
            {
                deq.push_front(std::pair(child,curr.second + 1));
            }
        }

    }
    out.close();
}

void get_scene_roots(std::vector<std::string> &roots,std::fstream &in)
{
    // The function gets the transform_id of all the objects rooted by the scene
    // They are used to make the scene hierarchy in the right order

    std::string roots_parser;
    while(in>>roots_parser)
    {
                                
        if(isdigit(roots_parser[0]))
            {
                roots_parser.erase(roots_parser.size()-1,1);
                roots.emplace_back(roots_parser);
            }

    }
}

void get_used_script_guid(std::set<std::string> &used_scripts,std::fstream &in)
{
    // The function gets the guid of the script used by the MonoBehaviour component

     std::string mono_parser;
     while(mono_parser != "guid:")
     {
          in>>mono_parser;
                                
     }
     in>>mono_parser;
     used_scripts.emplace(mono_parser.erase(mono_parser.length() - 1,1));
}

void get_transform_info(object &parse,std::fstream &in)
{
    // The function parses the transform component and extracts the following information
    // The transform_id
    // The transform_ids of all its children
    // The transform_id of its parent
    // In this function the order of the children is reversed to make it easier to build the scene hierarchy

    in>>parse.transform_id;
    parse.transform_id.erase(0,1);
    std::string transform_parser;
    while(in>>transform_parser)
    {
        if(transform_parser == "m_Children:")
        {
            std::string child_parser;
            while(in>>child_parser)
            {
                if(child_parser != "m_Father:")
                {
                    if(isdigit(child_parser[0]))
                    {
                        child_parser.erase(child_parser.size() - 1,1);
                        parse.children.emplace_back(child_parser);
                    }
                }
                else
                {
                    std::string father_parser;
                    in>>father_parser>>parse.father;
                    parse.father.erase(parse.father.size()-1,1);
                    break;
                }

                reverse(parse.children.begin(),parse.children.end());
            }
            break;
        }
    }
}

void get_object_name(bool &name_is_ok,std::fstream &in,object &parse)
{
    // The function get the full name of the objects

    if(!name_is_ok)
    {
        in>>parse.name;
        std::string name_parser;
        while(in>>name_parser)
        {
            if(name_parser != "m_TagString:")
                parse.name.append(std::string(" ").append(name_parser));
            else
                break;
        }
    }
    name_is_ok = true;
}

void get_scene_hierarchy(std::set<std::string> &used_scripts,const std::string scene_path,const std::string dir_path,const char* output_dir)
{
    // This is the main function that parses the yaml files
    // It gets all the information found in the object struct and outputs the scene hierarchy to the output directory
    // The function also outputs to used_scripts the script guids that appear in this scene

                std::vector<std::string> roots;
                std::map<std::string,object> scene_objects;
                std::string cpy_path = dir_path, content;
                std::fstream in(cpy_path.append(scene_path));

                while(in>>content)
                    {
                        if(content == "!u!1")
                        {
                            bool name_is_ok = false;
                            object parse;
                            in>>parse.id;
                            parse.id.erase(0,1);
                            std::string game_object_parser;

                            while(in>>game_object_parser)
                            {
                                if(game_object_parser == "m_Name:")
                                {
                                    get_object_name(name_is_ok,in,parse);
                                }

                                if(game_object_parser == "!u!4")
                                {
                                    get_transform_info(parse,in);
                                    break;
                                }
                            }
                            scene_objects[parse.transform_id] = parse;
                            
                        }

                        if(content == "m_Script:")
                        {
                           get_used_script_guid(used_scripts,in);
                        }

                        if(content == "m_Roots:")
                        {
                            get_scene_roots(roots,in);
                        }   
                    }
                    output_scene_hierarchy(roots,scene_objects,scene_path,output_dir);
}


int main(int argc, char* argv[])
{

    auto start = std::chrono::high_resolution_clock::now();

    if(argc != 3)
        return -1;
    else
    {   
        std::string path = std::string(argv[1]).append("/Assets/Scenes/");
        std::vector<std::string> scene_paths;
        get_scene_paths(scene_paths,path);


        std::vector<std::string> script_paths;
        std::string path2 = std::string(argv[1]).append("/Assets/Scripts/");
        get_script_meta_paths(script_paths,path2);

        std::map<std::string,std::string> script_guid_to_path;
        std::set<std::string> all_scripts;
        // Start a thread to get all the script guids and relative path
        // The results of it are need at the end so we can start while we procces the yaml files
        std::thread script_thread(get_script_guid,std::ref(all_scripts),std::ref(script_guid_to_path),script_paths,argv[1]);
        

        
        std::set<std::string> used_scripts;
        std::vector<std::thread> threads;
        for( int i = 0;i<scene_paths.size();i++)
            {
                // Start a thread for each yaml file
                threads.emplace_back(get_scene_hierarchy,std::ref(used_scripts),scene_paths[i],path,argv[2]);
            }

        script_thread.join();

        for(auto &thread :threads)
            thread.join();

        std::set<std::string> unused_scripts;

        // Obtain the unused_scripts by doing set difference between the set of all scripts and the set of used scripts
        std::set_difference(all_scripts.begin(),all_scripts.end(),used_scripts.begin(),used_scripts.end(),std::inserter(unused_scripts,unused_scripts.end()));
        
        // Open the filestream to the .csv and output the data
        std::ofstream out(std::string(argv[2]).append("/UnusedScripts.csv"));

        out<<"Relative Path,GUID"<<std::endl;

        for(auto script : unused_scripts)
            out<<script_guid_to_path[script]<<","<<script<<std::endl;


    }
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout<<duration.count()<<std::endl;
    
    return 0;
}