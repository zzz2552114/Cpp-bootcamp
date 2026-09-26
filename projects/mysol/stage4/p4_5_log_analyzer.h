#include<unordered_map>
#include<vector>
#include<string>
#include<set>
#include<map>
#include<algorithm>



namespace log5{
    using Counts = std::unordered_map<std::string, int>;

    Counts CountLevels(const std::vector<std::string> &logs){
        Counts mp;
        for(const auto &s:logs){
            ++mp[s];
        }
        return mp;
    }

    std::set<std::string> UniqueLevels(const std::vector<std::string> &logs){
        std::set<std::string> st(logs.begin(),logs.end());
        return st;
    }
    
    std::vector<std::string> Filter(const std::vector<std::string> &logs,
    const std::string &level)
    {   

        std::vector<std::string> res;
        for(auto &ele:logs){
            if(ele==level) res.push_back(ele);
        }
        return res;
    }


    void RemoveLevel(std::vector<std::string>& logs,const std::string& level){
        logs.erase(std::remove_if
            (logs.begin(),logs.end(),
            [&level](const std::string& ele){return ele == level;})
                    ,logs.end());
    }
    int CountOf(const Counts &c, const std::string &level){
        auto it = c.find(level);
        if(it!=c.end()) return it->second;
        return 0;
    }
    std::string Join(const std::set<std::string> &s){
        std::string res;
        for(auto &ele:s){
            res += ele;
            res += ' ';
        }
        size_t sz = res.size();
        if(sz>0) res = res.substr(0,sz-1);
        return res;
    }
    std::string Join(const std::vector<std::string> &v){
        std::string res;
        for (auto &ele : v)
        {
            res += ele;
            res += ' ';
        }
        size_t sz = res.size();
        if (sz > 0)
            res = res.substr(0, sz - 1);
        return res;
    }
}