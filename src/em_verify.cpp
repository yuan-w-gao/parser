//
// Created by Yuan Gao on 9/26/23.
//
#include <vector>
#include <unordered_map>
#include <random>

#include "manager.hpp"
#include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"
#include "em_framework/em.hpp"

using namespace shrg;

enum class SamplerError{
    sNone = 0,
    sRuleWeightMismatch = 1,
    sNoResult = 2,
    sNoRule = 3,
    sNoWeight = 4
};

constexpr int toInt(SamplerError e){
    switch(e){
    case SamplerError::sNone:
        return 0;
    case SamplerError::sRuleWeightMismatch:
        return 1;
    case SamplerError::sNoResult:
        return 2;
    case SamplerError::sNoRule:
        return 3;
    case SamplerError::sNoWeight:
        return 4;
    default:
        return -1;
    }
}

template <typename T> class Option{
  private :
    T* ptr;
    SamplerError error_code;

  public:
    explicit Option(T* p){
        if(p == nullptr){
            ptr = nullptr;
            error_code = SamplerError::sNoResult;
        }
        ptr = p;
        error_code = SamplerError::sNone;
    }
    explicit Option(SamplerError e){
        ptr = nullptr;
        error_code = e;
    }
    Option(){
        ptr=nullptr;
        error_code = SamplerError::sNoResult;
    }

    bool is_error(){
        if(ptr != nullptr && error_code == SamplerError::sNone){
            return false;
        }else{
            assert(toInt(error_code) >= 1 && toInt(error_code) <= 4);
            return true;
        }
    }

    SamplerError get_error(){
        return error_code;
    }

    T* getPtr(){
        if(!is_error()){
            return ptr;
        }else{
            return nullptr;
        }
    }
};


class RuleSampler{
  public:
    std::random_device rd;
    std::mt19937 gen;
    RuleMap rule_map;
    WeightMap weight_map;

    RuleSampler():gen(rd()){

    }

    RuleSampler(RuleMap rules, WeightMap weights){
        this->rule_map = rules; //change this to explicit copying
        this->weight_map = weights; // change this to explicit copying
    }

    bool addRule(SHRG *rule, double weight){
        Label label = rule->label;
        if(std::find(rule_map[label].begin(), rule_map[label].end(), rule) != rule_map[label].end()){
            return false;
        }
        rule_map[label].push_back(rule);
        weight_map[label].push_back(weight);

        assert(rule_map[label].size() == weight_map[label].size());
        int rule_index = find(rule_map[label].begin(), rule_map[label].end(), rule) - rule_map[label].begin();
        int weight_ind = find(weight_map[label].begin(), weight_map[label].end(), weight) - weight_map[label].begin();
        assert(rule_index == weight_ind);
        return true;
    }

    Option<SHRG> sampleWithLabel(Label label){
        std::vector<SHRG*> rules = rule_map[label];
        if(rules.empty()){
            return Option<SHRG>(SamplerError::sNoRule);
        }
        WeightVector weights = weight_map[label];
        if(weights.empty()){
            return Option<SHRG>(SamplerError::sNoWeight);
        }
        if(weights.size() != rules.size()){
            return Option<SHRG>(SamplerError::sRuleWeightMismatch);
        }

        if(rules.size() == 1){
            return Option<SHRG>(rules[0]);
        }

        //sampling
        std::discrete_distribution<> dist(weights.begin(), weights.end());
        int i = dist(gen);
        return Option<SHRG>(rules[i]);
    }

    ~RuleSampler()= default;


};

class treeNode{
  private:
    static const int VISITED = 1000;
    int visited = -1;
  public:
    SHRG *rule;
    std::vector<treeNode*> children;

    explicit treeNode(SHRG *r){
        rule = r;
    }

    treeNode(SHRG *r, std::vector<treeNode*> &c){
        rule = r;
        children = c;
    }

    void addChild(treeNode* c){
        children.push_back(c);
    }

    int countNodes(){
        if(visited == VISITED){
            return 0;
        }
        int total = 1;
        visited = VISITED;
        for(auto &child:children){
            total += child->countNodes();
        }
        return total;
    }

    ~treeNode(){
        for(auto &child:children){
            if(child == nullptr){
                continue;
            }
            child->~treeNode();
            delete child;
        }
    }
};

treeNode* buildGraph(RuleSampler &sampler, Label label){

    Option<SHRG> res = sampler.sampleWithLabel(label);
    if(res.is_error()){
        return nullptr;
    }
    SHRG* rule = res.getPtr();
    treeNode *node = new treeNode(rule);
    //check for boundary node?
    for(auto &edge:rule->nonterminal_edges){
         treeNode *child = buildGraph(sampler, edge->label);
         if(child == nullptr){
             return nullptr;
         }
         node->addChild(child);
    }

    return node;
}


int main(int argc, char *argv[]) {
    auto *manager = &Manager::manager;
    manager->Allocate(1);

    if (argc != 4) {
        LOG_ERROR("Usage: generate <parser_type> <grammar_path> <graph_path>");
        return 1;
    }

    manager->LoadGrammars(argv[2]);
    manager->LoadGraphs(argv[3]);

    auto &context = manager->contexts[0];
    context->Init(argv[1], false , 100 );
    auto generator = context->parser->GetGenerator();
    std::vector<SHRG *>shrg_rules = manager->shrg_rules;

    double lower_bound = 0.0;
    double upper_bound = 1.0;
    std::uniform_real_distribution<double> unif(lower_bound,upper_bound);
    std::default_random_engine re;



    RuleSampler sampler;
    std::unordered_map<Label, int> label_set;

    int k = 2;

    for(int i = 0; i < 100; i ++){
        double random_weight = unif(re);
        bool added = sampler.addRule(shrg_rules[i], random_weight);
        label_set[shrg_rules[i]->label]++;
    }

    std::cout << "label: " << shrg_rules[k]->label << ", count: " << label_set[shrg_rules[k]->label] << "\n";


    treeNode *tree = buildGraph(sampler, 84);

    std::cout << "number of nodes: " << tree->countNodes() << "\n";


    return 0;
}



