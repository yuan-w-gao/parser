//
// Created by Yuan Gao on 3/20/23.
//
//#include <queue>
//#include <map>
//#include <vector>
//#include <math.h>
//#include <string>
//#include <fstream>
//#include <iterator>
//
//#include "../graph_parser/parser_chart_item.hpp"
//#include "../manager.hpp"
//#include "inside_outside.hpp"
//
//namespace shrg{
//    const int VISITED = -2000;
//
//    //const double ChartItem::ZERO_LOG = 3000.0;
//    const double epsilon = 0.000000001;
//    using ParentTup = std::tuple<ChartItem*, std::vector<ChartItem*>>;
//    using RuleVector = std::vector<SHRG *>;
//    using LabelToRule = std::map<LabelHash, RuleVector>;
//    using LabelCount = std::map<LabelHash, double>;
//
//    struct LessThanByLevel
//    {
//        bool operator()(ChartItem *lhs, const ChartItem *rhs) const
//        {
//            return lhs->level > rhs->level;
//        }
//    };
//
//    typedef std::priority_queue<ChartItem*,std::vector<ChartItem*>, LessThanByLevel> NodeLevelPQ;
//
//
//    std::set<int> indices({ 15,19,23,54,59,60,69,28,41, 75,94,98,166
//                           ,115,119,122,127,128,131,142,147,148,151,162,175
//                           ,176,189,214,222,229,233,234,243,250,253,262,282
//                           ,284,287,292,296,336,338,341,346,350,392,393,394
//                           ,403,407,429,442,458,473,476,478,481,482,485,499
//                           ,502,510,511,525,532,539,532,539,584,599,603,606
//                           ,607,608,645,656,661,681,686,694,697,698,700,703
//                           ,706,708,711,724,744,754,792,806,809,822,835,839
//                           ,843,847,851,863,863,867,874,875,879,882,883,885,898
//                           ,902,906,907,927,935,946,950,960,967,978,997,1008
//                           ,1014,1016,1022,1026,1028,1031,1034,1045,1065,1068
//                           ,1070,1072,1074,1076,1077,1078,1080,1082,1096,1097
//                           ,1103,1106,1119,1123,1128,1131,1149,1151,1154,1155
//                           ,1168,1175,1180,1181,1210,1237,1242,1245,1252,1263
//                           ,1267,1269,1271,1295,1308,1324,1325,1332,1336,1345
//                           ,1349,1352,1359,1367,1371,1374,1375,1398,1399,1413
//                           ,1414,1415,1431,1432,1441,1443,1445,1447,1448,1459
//                           ,1463,1477,1480,1483,1487,1492,1495,1514,1520,1527
//                           ,1528,1530,1540,1542,1545,1547,1549,1550,1551,1552
//                           ,1564,1567,1568,1575,1578,1587,1600,1605,1618,1626
//                           ,1635,1645,1651,1654,1683,1685,1691,1694,1507
//    });
//
//    // ######################## Helper Functions ##############################
//
//    bool double_equals(double a, double b){
//        return abs(a - b) < epsilon;
//    }
//
//    //log(x + y)
//    //a = log(x), b = log(y)
//    double addLogs(double a, double b){
//        if(a == ChartItem::ZERO_LOG){
//            return b;
//        }
//        if(b == ChartItem::ZERO_LOG){
//            return a;
//        }
//
////        if(a == 0.0 || b == 0.0){
////            std::cout << "make zero log";
////        }
//
//        double min_ab = std::min(a, b);
//        a = std::max(a, b);
//        b = min_ab;
//
//        if (isinf(a) && isinf(b)) {
//            return -std::numeric_limits<double>::infinity();
//        } else if (isinf(a)) {
//            return b;
//        } else if (isinf(b)) {
//            return a;
//        } else {
//            return a + log1p(std::exp(b - a));
//        }
//    }
//
//    double minusLogs(double a, double b){
//        if(a == ChartItem::ZERO_LOG){
//            return a;
//        }
//        if(b == ChartItem::ZERO_LOG){
//            return a;
//        }
//
//        return std::log(std::exp(a) - std::exp(b));
//    }
//
//    double get_label_prob(std::map<LabelHash, double> &label_prob, LabelHash label){
//        auto res = label_prob.find(label);
//        if(res == label_prob.end()){
//            return ChartItem::ZERO_LOG;
//        }
//        return label_prob[label];
//    }
//
//    /*double addLogs(double a, double b) {
//        if(double_equals(a, ChartItem::ZERO_LOG)){
//            return b;
//        }
//        if(double_equals(b, ChartItem::ZERO_LOG)){
//            return a;
//        }
//        double max_val = std::max(a, b);
//        double shifted_a = a - max_val;
//        double shifted_b = b - max_val;
//
//        return std::log(std::exp(shifted_a) + std::exp(shifted_b)) + max_val;
//        //return std::log(std::exp(a) + std::exp(b));
//    }*/
//
//    void numNext(ChartItem *root){
//        int i = 0;
//        auto ptr = root;
//        do{
//            i ++;
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//        std::cout <<"num item: " << i << "\n";
//    }
//
//    LabelToRule getRuleDict(RuleVector &shrg_rules){
//        LabelToRule dict;
//        for(auto rule:shrg_rules){
//            dict[rule->label_hash].push_back(rule);
//        }
//
//        //remove duplicates
//        for(auto const& x:dict){
//            LabelHash l = x.first;
//            RuleVector v = x.second;
//            std::set<SHRG*> s(v.begin(), v.end());
//            v.assign(s.begin(), s.end());
//        }
//        return dict;
//    }
//
//    // ######################## Inside Outside Algo ###########################
//
//    double computeInside(ChartItem *root){
//        if(root->inside_visited_status == VISITED){
//            return root->log_inside_prob;
//        }
//        ChartItem *ptr = root;
//        double sum_log_prob = ChartItem::ZERO_LOG;
//
//        do{
//            SHRG* rule_ptr = ptr->rule_ptr;
//            double curr_log_prob = rule_ptr->log_rule_weight;
//
//            if(curr_log_prob > 0.0 && curr_log_prob != ChartItem::ZERO_LOG){
//                std::cout << "this shouldn't happen\n";
//            }
//
//            if(curr_log_prob == ChartItem::ZERO_LOG){
//                curr_log_prob = 0.0; //TODO: this needs changed
//            }
//
//            for(ChartItem* child:ptr->children){
//                curr_log_prob += computeInside(child);
//            }
//
//            sum_log_prob = addLogs(sum_log_prob, curr_log_prob);
//
//            if(sum_log_prob > 0.0 && sum_log_prob != ChartItem::ZERO_LOG){
//                std::cout << "inside positive\n";
//            }
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//
//        do{
//            if(sum_log_prob > 0.0 && sum_log_prob != ChartItem::ZERO_LOG){
//                std::cout << "inside positive\n";
//            }
//            ptr->log_inside_prob = sum_log_prob;
//            ptr->inside_visited_status = VISITED;
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//
//        return ptr->log_inside_prob;
//    }
//
//    void computeOutsideNode(ChartItem* root, NodeLevelPQ &queue){
//        if(root->outside_visited_status == VISITED){
//            return;
//        }
//
//        ChartItem *ptr = root;
//
//        //compute outside probability for each node
//        do{
//            // if a node doesn't have any parent pointers, we skip
//            // this also skips tree roots which don't have parents
//            // this is intended behavior as the roots have outside prob of 1.0
//            if(ptr->parents_sib.size() < 1){
//
//                ptr = ptr->next_ptr;
//                continue;
//            }
//            double sum_log_out = ChartItem::ZERO_LOG;
//            for(int i = 0; i < ptr->parents_sib.size(); i++){
//                ChartItem *parent = std::get<0>(ptr->parents_sib[i]);
//                std::vector<ChartItem*> siblings = std::get<1>(ptr->parents_sib[i]);
//
//                double curr_log_out = parent->rule_ptr->log_rule_weight;
//                if(curr_log_out > 0.0){
//                    std::cout << "this also shouldn't happen\n";
//                }
//
//                if(parent->log_outside_prob > 0.0){
//                    std::cout << "why is this positive?\n";
//                }
//
//                curr_log_out += parent->log_outside_prob;
//
//                for(ChartItem *sibling: siblings){
//                    curr_log_out += sibling->log_inside_prob;
//                }
//
//                sum_log_out = addLogs(sum_log_out, curr_log_out);
//                if(sum_log_out > 0){
//                    std::cout << "outside positive\n";
//                }
//            }
//            if(sum_log_out > 0){
//                std::cout << "outside positive\n";
//            }
//            ptr->log_outside_prob = sum_log_out;
//            ptr->outside_visited_status = VISITED;
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//        double root_log_outside = root->log_outside_prob;
//
//        // adds outside prob of root to nodes
//        // if it doesn't have any parent pointers
//        // AND adds children to the queue
//        do{
//            if(ptr->parents_sib.size() < 1 && ptr->outside_visited_status != VISITED
//                && !double_equals(0.0, ptr->log_outside_prob)){
//                if(root_log_outside > 0){
//                    std::cout << "positive\n";
//                }
//                ptr->log_outside_prob = root_log_outside;
//                ptr->outside_visited_status = VISITED;
//            }
//
//            for(ChartItem *child: ptr->children){
//                queue.push(child);
//            }
//            ptr->outside_visited_status = VISITED;
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//    }
//
//    void computeOutside(ChartItem *root){
//        NodeLevelPQ pq;
//        ChartItem *ptr = root;
//
//        do{
//            ptr->log_outside_prob = 0.0;
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//        pq.push(ptr);
//        do{
//            ChartItem *curr_node = pq.top();
//            computeOutsideNode(curr_node, pq);
//            pq.pop(); //todo: why pop after computation?
//        }while(!pq.empty());
//    }
//
////    void computeExpectedCount(ChartItem *root, double pw,
////                              std::map<LabelHash, double> &label_count){
////        if(root->count_visited_status == VISITED){
////            return;
////        }
////        ChartItem *ptr = root;
////
////        do{
////            double curr_log_count = ptr->rule_ptr->log_rule_weight;
////            if(curr_log_count > 0){
////                std::cout << "check this, why??\n";
////            }
////            curr_log_count += ptr->log_outside_prob;
////            curr_log_count -= pw;
////
////            double children_inside_total = 0.0;
////            for(ChartItem *child:ptr->children){
////                children_inside_total += child->log_inside_prob;
////            }
////            curr_log_count += children_inside_total;
////
////            ptr->log_sent_rule_count = addLogs(ptr->log_sent_rule_count, curr_log_count);
////            //ptr->log_sent_rule_count += std::exp(curr_log_count);
////            SHRG* rule_ptr = ptr->rule_ptr;
////            rule_ptr->log_count = addLogs(rule_ptr->log_count, curr_log_count);
////            //rule_ptr->log_count += std::exp(curr_log_count);
////            label_count[rule_ptr->label_hash] = addLogs(label_count[rule_ptr->label_hash], curr_log_count);
////            //label_count[rule_ptr->label_hash] += std::exp(curr_log_count);
////            if(!isnormal(label_count[rule_ptr->label_hash])){
////                std::cout << "label: " << rule_ptr->label_hash << ", count: ";
////                std::cout << label_count[rule_ptr->label_hash] << "\n";
////            }
////
////            ptr->count_visited_status = VISITED;
////
////            for(ChartItem *child:ptr->children){
////                computeExpectedCount(child, pw, label_count);
////            }
////
////
////            ptr = ptr->next_ptr;
////        }while(ptr != root);
////    }
//
//    void computeExpectedCount(ChartItem *root, double pw,
//                              std::map<LabelHash, double> &label_count){
//        if(root->count_visited_status == VISITED){
//            return;
//        }
//        ChartItem *ptr = root;
//
//        do{
//            double curr_log_count = ptr->rule_ptr->log_rule_weight;
//            if(curr_log_count > 0){
//                std::cout << "check this, why??\n";
//            }
//            curr_log_count += ptr->log_outside_prob;
//            curr_log_count -= pw;
//
//            double children_inside_total = 0.0;
//            for(ChartItem *child:ptr->children){
//                children_inside_total += child->log_inside_prob;
//            }
//            curr_log_count += children_inside_total;
//
//            SHRG* rule_ptr = ptr->rule_ptr;
//
//            ptr->log_sent_rule_count = addLogs(ptr->log_sent_rule_count, curr_log_count);
////            double curr_label_count = get_label_prob(label_count, rule_ptr->label_hash);
//            label_count[rule_ptr->label_hash] = addLogs(label_count[rule_ptr->label_hash], curr_log_count);
//            double rule_log_count = rule_ptr->log_count;
//            rule_ptr->log_count = addLogs(rule_ptr->log_count, curr_log_count);
//
//            if(!isnormal(label_count[rule_ptr->label_hash]) && label_count[rule_ptr->label_hash] != 0.0){
//                std::cout << "label: " << rule_ptr->label_hash << ", count: ";
//                std::cout << label_count[rule_ptr->label_hash] << "\n";
//            }
//
//            if(rule_ptr->log_count > label_count[rule_ptr->label_hash]){
//                std::cout << "what";
//            }
//
//            ptr->count_visited_status = VISITED;
//
//            for(ChartItem *child:ptr->children){
//                computeExpectedCount(child, pw, label_count);
//            }
//
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//    }
//
//    void computeExpectedCount_VB(ChartItem *root, double pw,
//                                 std::map<LabelHash, double> &label_count,
//                                 std::map<int, double> &rule_count,
//                                 bool first_parse){
//        if(root->count_visited_status == VISITED){
//            return;
//        }
//
//        ChartItem *ptr = root;
//        do{
//            double curr_log_count = ptr->rule_ptr->log_rule_weight;
//            if(curr_log_count > 0){
//                std::cout << "check this, why??\n";
//            }
//            curr_log_count += ptr->log_outside_prob;
//            curr_log_count -= pw;
//
//            double children_inside_total = 0.0;
//            for(ChartItem *child:ptr->children){
//                children_inside_total += child->log_inside_prob;
//            }
//            curr_log_count += children_inside_total;
//
//            SHRG* rule_ptr = ptr->rule_ptr;
//
//            if(!first_parse){
//
//            }
//            label_count[rule_ptr->label_hash] = minusLogs(label_count[rule_ptr->label_hash], ptr->log_sent_rule_count);
//            rule_ptr->log_count = minusLogs(rule_ptr->log_count, ptr->log_sent_rule_count);
//
//            ptr->log_sent_rule_count = curr_log_count;
//            rule_count[ptr->attrs_ptr->grammar_ptr->cfg_rules[0].shrg_index] = addLogs(rule_count[ptr->attrs_ptr->grammar_ptr->cfg_rules[0].shrg_index], curr_log_count);
//
//            ptr->count_visited_status = VISITED;
//
//            for(ChartItem *child:ptr->children){
//                computeExpectedCount_VB(root, pw, label_count, rule_count, first_parse);
//            }
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//
//        return ;
//    }
//
//    void writeHistoryToFile(std::string filename, std::vector<double> history[], int size){
//        std::ofstream outFile(filename);
//        for(int i = 0; i < size; i ++){
//            outFile << i;
//            for(int j = 0; j < history[i].size(); j ++){
//                outFile << "," << history[i][j];
//            }
//            outFile << "\n";
//        }
//        outFile.close();
//    }
//
//    void train(std::vector<SHRG*> &shrg_rules, std::vector<EdsGraph> &graphs,
//               Context *context, double threshold){
//        std::cout << "Training Time~ \n";
//        clock_t t1,t2;
//
//        auto generator = context->parser->GetGenerator();
//        double max_improve;
//        int iteration = 0;
//
//        std::vector<double> history[shrg_rules.size()];
//
//        for(int i = 0; i < shrg_rules.size(); i++){
//            history[i].push_back(shrg_rules[i]->log_rule_weight);
//        }
//
//        do{
//            t1 = clock();
//            std::map<LabelHash, double> label_count;
//
//            for(int i = 0; i < 2000; i++){
////                if(indices.find(i) != indices.end()){
////                    continue;
////                }
//
//
//                //std::cout << i << "\n";
//                if(i % 100 == 0){
//                  std::cout << i << "\n";
//                }
//
//                auto code = context->Parse(graphs[i]);
//
//                if(code == ParserError::kNone){
//                    ChartItem *root = context->parser->Result();
//                    preComputeIter(root, generator, shrg_rules);
//
//                    double pw = computeInside(root);
//                    computeOutside(root);
//                    computeExpectedCount(root, pw, label_count);
//                }
//            }
//
//            max_improve = -1.0;
//            for(auto i = 0; i < shrg_rules.size(); i++){
//                SHRG* rule = shrg_rules[i];
//                double curr_label_count = label_count[rule->label_hash];
//                double new_phi, improve;
//                if(rule->log_count == 0.0){
//                    new_phi = ChartItem::ZERO_LOG;
//                    if(rule->log_rule_weight == ChartItem::ZERO_LOG){
//                        improve = 0.0;
//                    }else{
//                        improve = abs(rule->log_rule_weight);
//                    }
//                }else{
//                    new_phi = rule->log_count - curr_label_count;
//                    improve = abs(new_phi - rule->log_rule_weight);
//                }
//
//                if(improve > 25){
//                    std::cout<< "";
//                }
//
//                if(improve > max_improve){
//                    max_improve = improve;
//                }
//
////                if(shrg_rules[2714]->count == 0.0){
////                    std::cout << "zero?";
////                }
//
//                if(!isnormal(new_phi)){
//                    //std::cout << "new weight for " << rule->label << ": " << new_phi << "\n";
//                }
//
//                if(new_phi > 0.0){
//                    //std::cout << "new weight for " << rule->label << ": " << new_phi << "\n";
//                }
//
//                rule->log_rule_weight = new_phi;
//                history[i].push_back(new_phi);
//
//                //rule->count = 0.0;
//            }
//            writeHistoryToFile("./weight_history_filtered", history, shrg_rules.size());
//            //zeroing out count
//            // this cannot be combined with the previous loop because
//            // shrg_rules contains duplicates
//            for(auto i = 0; i < shrg_rules.size(); i++){
//                shrg_rules[i]->log_count = 0.0;
//            }
//
//
//
//
//            t2 = clock();
//            double time_diff = (double)(t2 - t1)/CLOCKS_PER_SEC;
//            std::cout << "iteration: " << iteration << "\nimprove: " << max_improve;
//            std::cout << ", in " << time_diff << " seconds \n\n";
//            iteration++;
//        }while(max_improve > threshold);
//
//    }
//
//    void colllapsed_VB(std::vector<SHRG*> & shrg_rules, std::vector<EdsGraph> &graphs,
//                       Context *context, double threshold){
//        std::map<LabelHash, double> label_count;
//        auto generator = context->parser->GetGenerator();
//        double max_improve;
//        int iteration = 0;
//        do{
//            for(int i = 0; i < 200; i++){
//                auto code = context->Parse(graphs[i]);
//
//                if(code == ParserError::kNone){
//                    ChartItem *root = context->parser->Result();
//                    preComputeIter(root, generator, shrg_rules);
//                    std::map<int, double> rule_count;
//
//                    double pw = computeInside(root);
//                    computeOutside(root);
//                    computeExpectedCount_VB(root, pw, label_count, rule_count, iteration==0);
//
//                    for ( const auto &p : rule_count){
//                        SHRG* rule = shrg_rules[p.first];
//                        double new_phi, improve;
//                        double curr_label_count = label_count[rule->label_hash];
//                        if(rule->log_count == 0.0){
//                            new_phi = ChartItem::ZERO_LOG;
//                            if(rule->log_rule_weight == ChartItem::ZERO_LOG){
//                                improve = 0.0;
//                            }else{
//                                improve = abs(rule->log_rule_weight);
//                            }
//                        }else{
//                            new_phi = rule->log_count - curr_label_count;
//                            improve = abs(new_phi - rule->log_rule_weight);
//                        }
//                        rule->log_rule_weight = new_phi;
//                    }
//                    for ( const auto &p : rule_count){
//                        SHRG* rule = shrg_rules[p.first];
//                        rule->log_count = addLogs(rule->log_count, p.second);
//                        label_count[rule->label_hash] = addLogs(label_count[rule->label_hash], p.second);
//                    }
//
//                }
//            }
//            iteration ++;
//        }while(max_improve < threshold);
//    }
//
//    double incremental_onlineEMUpdate(ChartItem* root, std::map<LabelHash, double> &label_count, double increment){
//        if(root->update_status == VISITED){
//            return -0.1;
//        }
//        ChartItem *ptr = root;
//        double total_improve = -1;
//        do{
//            SHRG* rule = ptr->rule_ptr;
//            double curr_label_count = label_count[rule->label_hash];
//            double new_phi, improve;
//            if(rule->log_count == ChartItem::ZERO_LOG){
//                new_phi = ChartItem::ZERO_LOG;
//            }else {
//                new_phi = rule->log_count - curr_label_count;
//            }
//
//            if(new_phi != ChartItem::ZERO_LOG && new_phi > 0.0){
//                std::cout << "positive weight";
//            }
//            if(!isnormal(new_phi)){
//                std::cout << "";
//            }
////            double log_diff = addLogs(new_phi, -rule->log_rule_weight);
////
////            if(log_diff > 0){
////                std::cout << "";
////            }
////
////            rule->prev_rule_weight = rule->log_rule_weight;
////            rule->log_rule_weight = addLogs(rule->log_rule_weight, -addLogs(std::log(increment), log_diff));
//
//            double a = std::log(increment) + new_phi;
//            double b = std::log(1 - increment) + rule->log_rule_weight;
//            double weighted_average = addLogs(a, b);
//
//            improve = abs(rule->log_rule_weight - weighted_average);
//            total_improve += improve;
//
//            rule->prev_rule_weight = rule->log_rule_weight;
//            rule->log_rule_weight = weighted_average;
//
//            if(rule->log_rule_weight > 0){
//                std::cout << "";
//            }
//
//            ptr->update_status = VISITED;
//            for(ChartItem* child:ptr->children){
//                double cimprove = incremental_onlineEMUpdate(child, label_count, increment);
//                total_improve += cimprove;
//            }
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//        return total_improve;
//    }
//
//    void onlineEM(std::vector<SHRG*> &shrg_rules, std::vector<EdsGraph> &graphs,
//                  Context *context, double threshold, double increment){
//        std::cout << "Training Time~ \n";
//        clock_t t1,t2;
//
//
//        auto generator = context->parser->GetGenerator();
//        double max_improve;
//        int iteration = 0;
//        do{
//            t1 = clock();
//            for(int i = 0; i < 20; i++){
//                if(i % 20 == 0){
//                    std::cout << i << "\n";
//                }
//                auto code = context->Parse(graphs[i]);
//                if(code == ParserError::kNone){
//                    std::map<LabelHash, double> label_count;
//                    ChartItem *root = context->parser->Result();
//                    addParentPointerIter(root, generator, 0);
//                    addRulePointer_online(root, generator, shrg_rules);
//                    //preComputeIter(root, generator, shrg_rules);
//
//                    double pw = computeInside(root);
//                    computeOutside(root);
//                    computeExpectedCount(root, pw, label_count);
//
//                    double improve = incremental_onlineEMUpdate(root, label_count, increment);
//                    //std::cout << "graph " << i << ", improve: " << improve << "\n";
//                    if(improve > max_improve){
//                        max_improve = improve;
//                    }
//                }
//            }
//            t2 = clock();
//            double time_diff = (double)(t2 - t1)/CLOCKS_PER_SEC;
//            std::cout << "iteration: " << iteration << "\nimprove: " << max_improve;
//            std::cout << ", in " << time_diff << " seconds \n\n";
//            iteration ++;
//        }while(max_improve > threshold);
//    }
//
//    void batchEM_update(ChartItem* root, std::map<LabelHash, double> &label_count, int num_batch){
//
//    }
//
//    void batchEM(std::vector<SHRG*> &shrg_rules, std::vector<EdsGraph> &graphs,
//                 Context *context, double threshold, int batch_size){
//
//        auto generator = context->parser->GetGenerator();
//        double max_improve;
//        int iteration = 0;
//
//        int num_batches = (graphs.size() + batch_size - 1) / batch_size;
//
//        do{
//            for(int batch = 0; batch < num_batches; batch ++){
//                int num_graphs;
//                if(batch == num_batches - 1){
//                    num_graphs = graphs.size() % batch_size; //TODO: maybe divisible
//                }else{
//                    num_graphs = batch_size;
//                }
//
//                std::map<LabelHash, double> label_count;
//
//                for(int j = 0; j < num_graphs; j++){
//                    int ind = batch_size * batch + j;
//                    auto code = context->Parse(batch_size * batch + j);
//                    if(code == ParserError::kNone){
//                        ChartItem *root = context->parser->Result();
//                        addParentPointerIter(root, generator, 0);
//                        addRulePointer(root, generator, shrg_rules);
//
//                        double pw = computeInside(root);
//                        computeOutside(root);
//                        computeExpectedCount(root, pw, label_count);
//                    }
//                }
//
//                max_improve = -1.0;
//                for(auto i = 0; i < shrg_rules.size(); i++){
//                    SHRG *rule = shrg_rules[i];
//                    double curr_label_count = label_count[rule->label_hash];
//                    double new_phi, improve;
//                    if(rule->log_count == 0.0){
//                        new_phi = ChartItem::ZERO_LOG;
//                        if(rule->log_rule_weight == ChartItem::ZERO_LOG){
//                            improve = 0.0;
//                        }else{
//                            improve = abs(rule->log_rule_weight);
//                        }
//                    }else{
//                        new_phi = rule->log_count - curr_label_count;
//                        improve = abs(new_phi - rule->log_rule_weight);
//                    }
//                    if(improve > max_improve){
//                        max_improve = improve;
//                    }
//
//                    //                if(shrg_rules[2714]->count == 0.0){
//                    //                    std::cout << "zero?";
//                    //                }
//
//                    if(!isnormal(new_phi)){
//                        //std::cout << "new weight for " << rule->label << ": " << new_phi << "\n";
//                    }
//
//                    if(new_phi > 0.0){
//                        //std::cout << "new weight for " << rule->label << ": " << new_phi << "\n";
//                    }
//
//                    rule->log_rule_weight = new_phi;
//                }
//                for(auto i = 0; i < shrg_rules.size(); i++){
//                    shrg_rules[i]->log_count = ChartItem::ZERO_LOG;
//                }
//            }
//        }while(max_improve > threshold);
//    }
//
//    void preComputeIter(ChartItem *root, Generator *generator, std::vector<SHRG *> &shrg_rules){
//        auto ptr = root;
//        addParentPointerIter(ptr, generator, 0);
//
//        addRulePointer(root, generator, shrg_rules);
//    }
//
//
//    void addRulePointer(ChartItem *root, Generator *generator, std::vector<SHRG *> &shrg_rules){
//        if(root->rule_visited == VISITED){
//            return;
//        }
//        ChartItem *ptr = root;
//        do{
//            auto grammar_ptr = ptr->attrs_ptr->grammar_ptr;
//            int shrg_index = grammar_ptr->best_cfg_ptr->shrg_index;
//            ptr->rule_ptr = shrg_rules[shrg_index];
//            //ptr->rule_ptr->count = 0.0;
//
//            ptr->rule_visited = VISITED;
//            for(auto child:ptr->children){
//                addRulePointer(child, generator, shrg_rules);
//            }
//            /*for(auto edge_ptr:grammar_ptr->nonterminal_edges){
//                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
//                addRulePointer(child, generator, shrg_rules);
//            }*/
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//    }
//
//    void addRulePointer_online(ChartItem *root, Generator *generator, std::vector<SHRG *> &shrg_rules){
//        if(root->rule_visited == VISITED){
//            return;
//        }
//        ChartItem *ptr = root;
//        do{
//            auto grammar_ptr = ptr->attrs_ptr->grammar_ptr;
//            int shrg_index = grammar_ptr->best_cfg_ptr->shrg_index;
//            ptr->rule_ptr = shrg_rules[shrg_index];
//            ptr->rule_ptr->log_count = ChartItem::ZERO_LOG;
//            ptr->rule_visited = VISITED;
//            for(auto child:ptr->children){
//                addRulePointer_online(child, generator, shrg_rules);
//            }
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//    }
//
//
//    void addParentPointer(ChartItem *root, Generator *generator, int level){
//        auto ptr = root;
//        auto root_grammar = root->attrs_ptr->grammar_ptr;
//
//        if(level > root->level){
//            root->level = level;
//        }
//        root->log_outside_prob = ChartItem::ZERO_LOG;
//        root->log_sent_rule_count = ChartItem::ZERO_LOG;
//        for(auto edge_ptr:root_grammar->nonterminal_edges){
//            ChartItem *child = generator->FindChartItemByEdge(root, edge_ptr);
//            //child->parents.push_back(root);
//            root->children.push_back(child);
//            addParentPointer(child, generator,root->level + 1);
//        }
//        for(int i = 0; i < root->children.size(); i++){
//            std::vector<ChartItem*> sib;
//            std::tuple<ChartItem*, std::vector<ChartItem*>> res;
//            for(int j = 0; j < root->children.size(); j++){
//                if(j == i){
//                    continue;
//                }
//                sib.push_back(root->children[j]);
//            }
//            if(root){
//                res = std::make_tuple(root, sib);
//                root->children[i]->parents_sib.push_back(res);
//            }
//        }
//    }
//
//    void addParentPointerIter(ChartItem *root, Generator *generator, int level){
//        auto ptr = root;
//        do{
//            auto ptr_grammar = ptr->attrs_ptr->grammar_ptr;
//
//            if(level > ptr->level){
//                ptr->level = level;
//            }
//            ptr->log_outside_prob = ChartItem::ZERO_LOG;
//            ptr->log_sent_rule_count = ChartItem::ZERO_LOG;
//
//
//
//            if(ptr->child_visited_status != VISITED){
//                for(auto edge_ptr:ptr_grammar->nonterminal_edges){
//                    ChartItem* child = generator->FindChartItemByEdge(ptr, edge_ptr);
//                    ptr->children.push_back(child);
//                    addParentPointerIter(child, generator, ptr->level + 1);
//                }
//                for(int i = 0; i < ptr->children.size(); i++){
//                    std::vector<ChartItem*> sib;
//                    std::tuple<ChartItem*, std::vector<ChartItem*>> res;
//                    for(int j = 0; j < ptr->children.size(); j++){
//                        if(j == i){
//                            continue;
//                        }
//                        sib.push_back(ptr->children[j]);
//                    }
////                    if(!ptr){
////                        std::cout << "Null pointer??";
////                    }
//                    if(ptr){
//                        res = std::make_tuple(ptr, sib);
//                        ptr->children[i]->parents_sib.push_back(res);
//                    }
//                }
//                ptr->child_visited_status = VISITED;
//            }else{
//                for(auto child:ptr->children){
//                    addParentPointerIter(child, generator, ptr->level + 1);
//                }
//            }
//            assert(ptr->children.size() == ptr_grammar->nonterminal_edges.size());
//
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//    }





    /*double computeInside(ChartItem *root){
        if(root->inside_visited_status == VISITED){
            return root->log_inside_prob;
        }

        double sum_log_prob = ChartItem::ZERO_LOG;
        ChartItem *ptr = root;

        do{
            auto *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            auto rule_ptr = ptr->rule_ptr;
            double curr_log_prob = rule_ptr->log_rule_weight;
            if(curr_log_prob == 0.0){
                std::cout << "grammar prob zero\n";
            }
            for(auto child:ptr->children){
                curr_log_prob += computeInside(child);
            }
            sum_log_prob = addLogs(sum_log_prob, curr_log_prob);

            //sum_prob += std::exp(curr_log_prob);
            ptr = ptr->next_ptr;
        }while (ptr != root);

        do{
            //ptr->inside_prob = std::log(sum_prob);
            if(!isnormal(sum_log_prob)){
                if(sum_log_prob == 0.0){
                    std::cout << "inside zero\n";
                }else {
                    std::cout << "inside underflow?" << sum_log_prob << "\n";
                }
            }
            ptr->log_inside_prob = sum_log_prob;
            ptr->inside_visited_status = VISITED;
            ptr = ptr->next_ptr;
        }while(ptr != root);

        return root->log_inside_prob;
    }*/



    /*void computeOutsideNode(Generator *generator, ChartItem *root,
                            std::queue<ChartItem*> *queue){
        double curr_outside_prob = root->log_outside_prob;
        ChartItem *ptr = root;

        do{
            auto grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            auto rule_ptr = ptr->rule_ptr;
            double grammar_prob = rule_ptr->log_rule_weight;

            for(auto edge_ptr: grammar_ptr -> nonterminal_edges){
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                double log_prob = grammar_prob + std::log(curr_outside_prob) + child->log_inside_prob;
                child->log_outside_prob += std::exp(log_prob);
            }

            ptr = ptr->next_ptr;
        }while(ptr != root);

        do{
            const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            for(auto edge_ptr:grammar_ptr->nonterminal_edges){
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                queue->push(child);
            }
            ptr = ptr->next_ptr;
        }while(ptr != root);
    }*/

    /*void computeOutside(Generator *generator, ChartItem *root_ptr){
        std::queue<ChartItem *> q;

        q.push(root_ptr);

        ChartItem *ptr = root_ptr;
        do{
            ptr->log_outside_prob = 1.0;
            ptr = ptr->next_ptr;
        }while(ptr != root_ptr);

        int item = 0;

        do{
            item ++;
            //std::cout << "loop " << item << "\n";

            ChartItem *curr_node = q.front();

            computeOutsideNode(generator, curr_node, &q);

            q.pop();
        }while(!q.empty());
    }*/



    /*void computeOutside2Node(Generator *generator, ChartItem *root, NodeLevelPQ *queue){
        if(root->outside_visited_status == VISITED){
            return ;
        }

        ChartItem *ptr = root;
        ptr->outside_visited_status = VISITED;
        if(ptr->parents_sib.size() == 0 && !double_equals(0.0, ptr->log_outside_prob)){
            std::cout << "here";
        }

        for(int i = 0; i < ptr->parents_sib.size(); i++){
            double curr_out = 0.0;
            ChartItem *parent = std::get<0>(ptr->parents_sib[i]);
            std::vector<ChartItem*> siblings = std::get<1>(ptr->parents_sib[i]);

            double sibling_inside = 0.0;
            for(auto sibling:siblings){
                sibling_inside += sibling->log_inside_prob;
            }

            curr_out += parent->rule_ptr->log_rule_weight;
            curr_out += parent->log_outside_prob;
            curr_out += ptr->log_inside_prob;
            curr_out += sibling_inside;

            if(!isnormal(curr_out)){
                std::cout << "outside Nan";
            }
            if(curr_out > 0.0){
                std::cout << "curr_out positive\n";
            }

            double outside = addLogs(ptr->log_outside_prob, curr_out);
            if(!isnormal(outside)){
                std::cout << "underflow??";
            }
            if(outside > 0.0){
                std::cout << "outside positive\n";
            }

            ptr->log_outside_prob = addLogs(ptr->log_outside_prob, curr_out);
        }

        auto grammar_ptr = ptr->attrs_ptr->grammar_ptr;
        for(auto child:ptr->children){
            queue->push(child);
        }
        return ;
    }*/

    /*void computeOutside2NodeIter(Generator *generator, ChartItem *root, NodeLevelPQ *queue){
        if(root->outside_visited_status == VISITED){
            return ;
        }

        ChartItem *ptr = root;
        do{
            ptr->outside_visited_status = VISITED;
            if(ptr->parents_sib.size() == 0 && !double_equals(0.0, ptr->log_outside_prob)){
                std::cout << "here";
            }

            for(int i = 0; i < ptr->parents_sib.size(); i++){
                double curr_out = 0.0;
                ChartItem *parent = std::get<0>(ptr->parents_sib[i]);
                std::vector<ChartItem*> siblings = std::get<1>(ptr->parents_sib[i]);

                double sibling_inside = 0.0;
                for(auto sibling:siblings){
                    sibling_inside += sibling->log_inside_prob;
                }

                curr_out += parent->rule_ptr->log_rule_weight;
                curr_out += parent->log_outside_prob;
                curr_out += ptr->log_inside_prob;
                curr_out += sibling_inside;

                if(!isnormal(curr_out)){
                    std::cout << "outside Nan";
                }
                if(curr_out > 0.0){
                    std::cout << "curr_out positive\n";
                }

                double outside = addLogs(ptr->log_outside_prob, curr_out);
                if(!isnormal(outside)){
                    std::cout << "underflow??";
                }

                if(outside > 0.0){
                    std::cout << "outside positive\n";
                }

                ptr->log_outside_prob = outside;
            }

            auto grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            for(auto child:ptr->children){
                queue->push(child);
            }
            ptr = ptr->next_ptr;
        }while(ptr != root);

        return ;
    }*/



    /*void computeOutside2(Generator *generator, ChartItem *root_ptr){
        NodeLevelPQ pq;


        ChartItem *ptr = root_ptr;
        do{
            ptr->log_outside_prob = 0.0;
            pq.push(ptr);
            ptr = ptr->next_ptr;
        }while(ptr != root_ptr);

        int item = 0;

        do{
            item ++;
            //std::cout << "loop " << item << "\n";

            ChartItem *curr_node = pq.top();

            computeOutside2Node(generator, curr_node, &pq);

            pq.pop();
        }while(!pq.empty());
    }*/

    /*void computeOutsideNodeSync(Generator *generator, ChartItem *root, NodeLevelPQ *queue){
        if(root->outside_visited_status == VISITED){
            return;
        }

        ChartItem *ptr = root;
        double root_outside_prob = 0.0;
        do{
            if(ptr->parents_sib.size() < 1 && ptr->outside_visited_status != VISITED){
                ptr = ptr->next_ptr;
                continue;
            }

            for(int i = 0; i < ptr->parents_sib.size(); i++){
                double curr_out = 0.0;
                ChartItem *parent = std::get<0>(ptr->parents_sib[i]);
                std::vector<ChartItem*> siblings = std::get<1>(ptr->parents_sib[i]);

                double sibling_sum_inside = 0.0;
                for(auto sibling:siblings){
                    sibling_sum_inside += sibling->log_inside_prob;
                }

                curr_out += parent->rule_ptr->log_rule_weight;
                curr_out += parent->log_outside_prob;
                curr_out += ptr->log_inside_prob; // this is wrong, why add inside of current pointer?
                curr_out += sibling_sum_inside;

                if(!isnormal(curr_out)){
                    std::cout << "outside Nan";
                }
                if(curr_out > 0.0){
                    std::cout << "curr_out positive\n";
                }

                double outside = addLogs(ptr->log_outside_prob, curr_out);
                if(!isnormal(outside)){
                    std::cout << "underflow??";
                }

                if(outside > 0.0){
                    std::cout << "outside positive\n";
                }

                ptr->log_outside_prob = outside;
            }

            if(ptr == root){
                root_outside_prob = ptr->log_outside_prob;
            }

            ptr->outside_visited_status = VISITED;
            ptr = ptr->next_ptr;
        }while(ptr != root);

        do{
            if(ptr->outside_visited_status != VISITED && ptr->parents_sib.size() < 1
                && !double_equals(0.0, ptr->log_outside_prob)){
                ptr->log_outside_prob = root_outside_prob;
                ptr->outside_visited_status = VISITED;
            }
            for(auto child:ptr->children){
                queue->push(child);
            }
            ptr = ptr->next_ptr;
        }while(ptr != root);
    }*/

    /*void computeOutside2Iter(Generator *generator, ChartItem *root_ptr){
        NodeLevelPQ pq;


        ChartItem *ptr = root_ptr;

        do{
            ptr->log_outside_prob = 0.0;
            ptr = ptr->next_ptr;
        }while(ptr != root_ptr);
        pq.push(ptr);

        int item = 0;

        do{
            item ++;
            //std::cout << "loop " << item << "\n";

            ChartItem *curr_node = pq.top();

            computeOutsideNodeSync(generator, curr_node, &pq);

            pq.pop();
        }while(!pq.empty());
    }*/

//    struct doubleDefaultedToZeroLog
//    {
//        double i = ChartItem::ZERO_LOG;
//
//        operator double() const {return i;}
//    };



    /*for(auto child:ptr->children){
                ptr->subgraph_prob= addLogs(ptr->subgraph_prob, grammar_prob + ptr->outside_prob + child->inside_prob - pw);
                ptr->count_visited = VISITED;
                trainSingleSentence(generator, child, pw, label_prob);
            }*/
    /*for(auto edge_ptr:grammar_ptr->nonterminal_edges){
        ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);

    ptr->subgraph_prob= addLogs(ptr->subgraph_prob, grammar_prob + ptr->outside_prob + child->inside_prob - pw);

    trainSingleSentence(generator, child, pw, label_prob);
    }*/

    /*void trainSingleSentence(Generator *generator, ChartItem *root_ptr, double pw, std::map<Label, double> &label_prob){
        if(root_ptr->count_visited == VISITED){
            return;
        }


        auto ptr = root_ptr;
        do{
            auto rule_ptr = ptr->rule_ptr;
            double grammar_prob = rule_ptr->log_rule_weight;

            double curr_log_count = grammar_prob + ptr->log_outside_prob;
            for(auto child:ptr->children){
                curr_log_count += child->log_inside_prob;
                trainSingleSentence(generator, child, pw, label_prob);
            }
            if(curr_log_count > 0.0){
                //std::cout << "log count positive\n";
            }
            ptr->sent_rule_count = addLogs(ptr->sent_rule_count, curr_log_count);
            ptr->temp_visit += 1;
            ptr->count_visited = VISITED;


            ptr = ptr->next_ptr;
        }while(ptr != root_ptr);

        do{
            //const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            auto rule_ptr = ptr->rule_ptr;
            rule_ptr->count = addLogs(rule_ptr->count, ptr->sent_rule_count);
            double curr_label_prob = get_label_prob(label_prob, rule_ptr->label);
            double temp = addLogs(curr_label_prob, ptr->sent_rule_count);
            if(temp > 0.0){
                //std::cout << "log prob positive\n";
            }
            if(std::isnan(temp)){
                //std::cout << "aa!";
            }
            label_prob[rule_ptr->label] = temp;
            //label_prob[grammar_ptr->label] += ptr->subgraph_prob;
            ptr = ptr->next_ptr;
        }while(ptr != root_ptr);
    }*/



    /*double trainIterate(Generator *generator, std::vector<ChartItem *> &forests, std::vector<SHRG*> &shrg_rules){
        std::map<Label, double> label_prob;
        int count = 0;
        for(auto root: forests){
            //std::cout << count;
            double pw = computeInside(root);
            computeOutside2(generator, root);
            trainSingleSentence(generator, root, pw, label_prob);
        }
        double max_improve = -0.1;
        for(auto rule:shrg_rules){
            double new_phi = rule->count - label_prob[rule->label];
            double improve = abs(new_phi - rule->log_rule_weight);
            if(improve > max_improve){
                max_improve = improve;
            }
            if(!isnormal(new_phi)){
                //std::cout << "grammar prob wrong? \n";
            }
            rule->log_rule_weight = new_phi;
            rule->count = ChartItem::ZERO_LOG;
        }

        return max_improve;
    }*/

    /*void train(std::vector<SHRG*> &shrg_rules, std::vector<EdsGraph> &graphs, Context *context, double threshold){
        std::cout << "Training Time~ \n";
        clock_t t1,t2;

        auto generator = context->parser->GetGenerator();

        int c = 0;
        double max_improve;
        do{
            t1 = clock();

            std::map<Label, double> label_prob;

            for(int i = 0; i < graphs.size(); i++){
                if(indices.find(i) != indices.end()){
                    continue;
                }

                if(i % 5 == 0){
                    std::cout << i << "\n";
                }

                auto code = context->Parse(i);

                if(code == ParserError::kNone){
                    ChartItem *root = context->parser->Result();
                    preComputeIter(root, generator, shrg_rules);

                    double pw = computeInside(root);
                    computeOutside2Iter(generator, root);
                    trainSingleSentence(generator, root, pw, label_prob);
                }
            }

            max_improve = -0.1;

            int counter = 0;

            for(auto rule:shrg_rules){

                double curr_label_prob = get_label_prob(label_prob, rule->label);
                //double new_phi = rule->count - label_prob[rule->label];
                double new_phi = rule->count - curr_label_prob;
                double improve = abs(new_phi - rule->log_rule_weight);
                if(improve > max_improve){
                    max_improve = improve;
                }

                if(!isnormal(new_phi)){
                    std::cout << "grammar prob wrong? \n";
                }
                rule->log_rule_weight = new_phi;
                rule->count = ChartItem::ZERO_LOG;

                counter ++;
            }

            //max_improve = trainIterate(generator, forests, shrg_rules);
            c++;
            t2 = clock();
            double time_diff = (double)(t2 - t1)/CLOCKS_PER_SEC;
            std::cout << "iteration: " << c << "\nimprove: " << max_improve << ", in " << time_diff << " seconds \n\n";
        }while(max_improve > threshold);

        return;
    }*/

//    void preCompute(ChartItem *root, Generator *generator, std::vector<SHRG *> &shrg_rules){
//        auto ptr = root;
//        do{
//            addParentPointer(ptr, generator, 0);
//            ptr = ptr->next_ptr;
//        }while(ptr != root);
//
//        addRulePointer(root, generator, shrg_rules);
//    }



    /*void syncParentPointer(ChartItem* root, Generator *generator){
        ChartItem* ptr = root;
        auto root_parent = root->parents_sib;
        do{
            if(ptr->parents_sib.size() > 0 || root_parent.size() == 0){
                for(auto child:ptr->children){
                    syncParentPointer(child, generator);
                }
                ptr = ptr->next_ptr;
                continue;
            }
            for(int i = 0; i < root_parent.size(); i++){
                auto tup = root_parent[i];
                ChartItem *parent;
                std::vector<ChartItem*>siblings;
                std::tie(parent, siblings) = *tup;
                //ChartItem *parent = std::get<0>(*root_parent[i]);
                //std::vector<ChartItem*> siblings = std::get<1>(*root_parent[i]);
                std::tuple<ChartItem*, std::vector<ChartItem*>> res = std::make_tuple(parent, siblings);
                ptr->parents_sib.push_back(&res);
            }
            for(auto child:ptr->children){
                syncParentPointer(child, generator);
            }
            ptr = ptr->next_ptr;
        }while(ptr != root);
    }*/
//}

//
// Created by Yuan Gao on 18/01/2024.
//
#include <queue>
#include <set>
#include <fstream>

#include "../manager.hpp"
#include "../graph_parser/parser_chart_item.hpp"

namespace shrg{

//static constexpr auto infinity = std::numeric_limits<double>::infinity();
static const int VISITED = -2000;
using ParentTup = std::tuple<ChartItem*, std::vector<ChartItem*>>;
using RuleVector = std::vector<SHRG *>;
using LabelToRule = std::map<LabelHash, RuleVector>;
using LabelCount = std::map<LabelHash, double>;

bool is_negative(double log_prob);
bool is_parent_computed(ChartItem *parent);
bool is_normal_count(double c);

struct LessThanByLevel
{
    bool operator()(ChartItem *lhs, const ChartItem *rhs) const
    {
        return lhs->level > rhs->level;
    }
};

typedef std::priority_queue<ChartItem*,std::vector<ChartItem*>, LessThanByLevel> NodeLevelPQ;

// ############################## Helper Functions #########################
double addLogs(double a, double b){
    if(a == ChartItem::log_zero){
        return b;
    }
    if (b == ChartItem::log_zero)
        return a;
    if(a > b){
        return a + log1p(exp(b - a));
    }
    return b + log1p(exp(a - b));
    //    double min = std::min(a, b);
    //    a = std::max(a, b);
    //    b = min;
    //
    //    return a + log1p(std::exp(b - a));
}

double minusLogs(double a, double b){
    if(b == -ChartItem::log_zero){
        return a;
    }
    if(a == -ChartItem::log_zero){
        assert(false);
    }
    assert(a >= b);
    return a + log1p(-exp(b - a));
}

ChartItem* getParent(ParentTup &p){
    return std::get<0>(p);
}

std::vector<ChartItem*> getSiblings(ParentTup &p){
    return std::get<1>(p);
}

LabelToRule getRuleDict(RuleVector &shrg_rules){
    LabelToRule dict;
    for(auto rule:shrg_rules){
        dict[rule->label_hash].push_back(rule);
    }

    //remove duplicates
    LabelToRule::iterator it;
    for(it = dict.begin(); it != dict.end(); it++){
        std::set<SHRG*>s(it->second.begin(), it->second.end());
        dict[it->first] = RuleVector(s.begin(), s.end());
    }
    return dict;
}

void clearRuleCount(RuleVector &shrg_rules){
    for(auto rule:shrg_rules){
        rule->log_count = ChartItem::log_zero;
    }
}

void setInitialWeights(LabelToRule &dict){
    LabelToRule::iterator it;
    for(it = dict.begin(); it != dict.end(); it++){
        RuleVector v = it->second;
        for(auto r:v){
            r->log_rule_weight = std::log(1.0/v.size());
        }
    }
}

void writeHistoryToFile(std::string filename, std::vector<double> history[], int size){
    std::ofstream outFile(filename);
    for(int i = 0; i < size; i ++){
        outFile << i;
        for(int j = 0; j < history[i].size(); j ++){
            outFile << "," << history[i][j];
        }
        outFile << "\n";
    }
    outFile.close();
}

// ############################## ALGO#######################

void addParentPointer(ChartItem *root, Generator *generator, int level){
    ChartItem *ptr = root;

    do{
        if(level > ptr->level){
            ptr->level = level;
        }

        const SHRG* rule = ptr->attrs_ptr->grammar_ptr;
        if(ptr->child_visited_status != VISITED){
            for(auto edge_ptr:rule->nonterminal_edges){
                ChartItem* child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                addParentPointer(child, generator, ptr->level + 1);
            }
            for(int i = 0; i < ptr->children.size(); i++){
                std::vector<ChartItem*> sib;
                std::tuple<ChartItem*, std::vector<ChartItem*>> res;
                for(int j = 0; j < ptr->children.size(); j++){
                    if(j == i){
                        continue;
                    }
                    sib.push_back(ptr->children[j]);
                }

                if(ptr){
                    res = std::make_tuple(ptr, sib);
                    ptr->children[i]->parents_sib.push_back(res);
                }
            }
            ptr->child_visited_status = VISITED;
        }else{
            for(auto child:ptr->children){
                addParentPointer(child, generator, ptr->level + 1);
            }
        }
        assert(ptr->children.size() == rule->nonterminal_edges.size());

        ptr = ptr->next_ptr;
    }while(ptr != root);
}

void addRulePointer(ChartItem *root, RuleVector &shrg_rules){
    if(root->rule_visited == VISITED){
        return;
    }

    ChartItem *ptr = root;
    do{
        auto grammar_index = ptr->attrs_ptr->grammar_ptr->best_cfg_ptr->shrg_index;
        ptr->rule_ptr = shrg_rules[grammar_index];

        ptr->rule_visited = VISITED;
        for(ChartItem *child:ptr->children){
            addRulePointer(child, shrg_rules);
        }
        ptr = ptr->next_ptr;
    }while(ptr != root);
}


double computeInside(ChartItem *root){
    if(root->inside_visited_status == VISITED){
        return root->log_inside_prob;
    }

    ChartItem *ptr = root;
    double log_inside = ChartItem::log_zero;

    do{
        double curr_log_inside = ptr->rule_ptr->log_rule_weight;
        assert(is_negative(curr_log_inside));

        double log_children = 0.0;
        for(ChartItem *child:ptr->children){
            log_children += computeInside(child);
        }
        assert(is_negative(log_children));

        curr_log_inside += log_children;
        assert(is_negative(curr_log_inside));

        log_inside = addLogs(log_inside, curr_log_inside);
        assert(is_negative(log_inside));

        ptr = ptr->next_ptr;
    }while(ptr != root);

    do{
        is_negative(log_inside);
        ptr->log_inside_prob = log_inside;
        ptr->inside_visited_status = VISITED;
        ptr = ptr->next_ptr;
    }while(ptr != root);

    return log_inside;
}

void computeOutsideNode(ChartItem *root, NodeLevelPQ &pq){
    ChartItem *ptr = root;

    do{
        if(ptr->parents_sib.empty()){
            ptr = ptr->next_ptr;
            continue;
        }

        double log_outside = ChartItem::log_zero;

        for(ParentTup &parent_sib : ptr->parents_sib){
            ChartItem *parent = getParent(parent_sib);
            std::vector<ChartItem*> siblings = getSiblings(parent_sib);
            assert(is_parent_computed(parent));

            double curr_log_outside = parent->rule_ptr->log_rule_weight;
            assert(is_negative(curr_log_outside));
            curr_log_outside += parent->log_outside_prob;
            assert(is_negative(curr_log_outside));

            for(auto sib:siblings){
                curr_log_outside += sib->log_inside_prob;
                assert(is_negative(curr_log_outside));
            }

            log_outside = addLogs(log_outside, curr_log_outside);
            assert(is_negative(log_outside));
        }
        ptr->log_outside_prob = log_outside;
        ptr->outside_visited_status = VISITED;


        ptr = ptr->next_ptr;
    }while(ptr != root);

    double root_log_outside = root->log_outside_prob;

    do{
        if(ptr->outside_visited_status != VISITED){
            ptr->log_outside_prob = root_log_outside;
            ptr->outside_visited_status = VISITED;
        }

        for(ChartItem *child:ptr->children){
            pq.push(child);
        }

        ptr = ptr->next_ptr;
    }while(ptr != root);
}

void computeOutside(ChartItem *root){
    NodeLevelPQ pq;
    ChartItem *ptr = root;

    do{
        ptr->log_outside_prob = 0.0;
        ptr = ptr->next_ptr;
    }while(ptr != root);

    pq.push(ptr);

    do{
        ChartItem *node = pq.top();
        computeOutsideNode(node, pq);
        pq.pop();
    }while(!pq.empty());
}

void computeExpectedCount(ChartItem *root, double pw){
    if(root->count_visited_status == VISITED){
        return ;
    }
    ChartItem *ptr = root;

    do{
        double curr_log_count = ptr->rule_ptr->log_rule_weight;
        assert(is_negative(curr_log_count));

        curr_log_count += ptr->log_outside_prob;
        curr_log_count -= pw;

        for(ChartItem *child:ptr->children){
            curr_log_count += child->log_inside_prob;
        }

        ptr->log_sent_rule_count = curr_log_count;
        ptr->rule_ptr->log_count = addLogs(ptr->rule_ptr->log_count, curr_log_count);

        ptr->count_visited_status = VISITED;
        for(ChartItem *child:ptr->children){
            computeExpectedCount(child, pw);
        }
        ptr = ptr->next_ptr;
    }while(ptr != root);
}

void EMUpdate(RuleVector &shrg_rules, LabelToRule &rule_dict){
    LabelCount total_count;
    LabelToRule::iterator it;
    for(it = rule_dict.begin(); it != rule_dict.end(); it++){
        RuleVector v = it->second;
        double log_total_count = ChartItem::log_zero;
        for(auto rule:v){
            log_total_count = addLogs(log_total_count, rule->log_count);
            assert(is_normal_count(log_total_count));
        }
        total_count[it->first] = log_total_count;
    }

    double ll = ChartItem::log_zero;
    for(auto rule:shrg_rules) {
        LabelHash l = rule->label_hash;
        double new_phi;

        // special case 1: the rule doesn't appear
        if (rule->log_count == ChartItem::log_zero) {
            // if the rule is the only one with the label
            if (rule_dict[l].size() == 1) {
                new_phi = 0.0;
            } else {
                new_phi = ChartItem::log_zero;
            }
        } else {
            new_phi = rule->log_count - total_count[l];
            assert(is_negative(new_phi));
        }

        //        double improve;
        //        if(new_phi == ChartItem::log_zero){
        //            improve = rule->log_rule_weight;
        //        }else{
        //            improve = abs(new_phi - rule->log_rule_weight);
        //        }
        //        if(improve > max_improve){
        //            max_improve = improve;
        //        }
        //
        rule->log_rule_weight = new_phi;
    }

//
//    return max_improve;
}

ChartItem* deepCopyChartItem(ChartItem* root) {
    if (!root) return nullptr;

    ChartItem* newRoot = new ChartItem(*root);
    std::unordered_map<ChartItem*, ChartItem*> oldToNew;
    oldToNew[root] = newRoot;

    std::queue<ChartItem*> queue;
    queue.push(root);

    while (!queue.empty()) {
        ChartItem* current = queue.front();
        queue.pop();

        for (auto child : current->children) {
            if (oldToNew.find(child) == oldToNew.end()) {
                oldToNew[child] = new ChartItem(*child);
                queue.push(child);
            }
            oldToNew[current]->children.push_back(oldToNew[child]);
        }

        if (current->next_ptr && oldToNew.find(current->next_ptr) == oldToNew.end()) {
            oldToNew[current->next_ptr] = new ChartItem(*current->next_ptr);
            queue.push(current->next_ptr);
        }
        oldToNew[current]->next_ptr = oldToNew[current->next_ptr];
    }

    return newRoot;
}

void EM(RuleVector &shrg_rules, std::vector<EdsGraph> &graphs,
        Context *context, double threshold){
    std::cout << "Training Time~ \n";
    clock_t t1,t2;
    int training_size = graphs.size();

    Generator *generator = context->parser->GetGenerator();
    int iteration = 0;
    double ll = ChartItem::log_zero;
    std::vector<double> history[shrg_rules.size()];
//    std::vector<double> history_graph_ll[training_size];
//    for(int i = 0; i < shrg_rules.size(); i++){
//        history[i].push_back(shrg_rules[i]->log_rule_weight);
//    }
    std::vector<ChartItem*> forests;
    LabelToRule rule_dict = getRuleDict(shrg_rules);
    setInitialWeights(rule_dict);
    for (int i = 0; i < training_size; i++) {
        EdsGraph graph = graphs[i];
        auto code = context->Parse(graph);
        if(code == ParserError::kNone) {
            ChartItem *root = context->parser->Result();
            ChartItem *copied_root = deepCopyChartItem(root);
            forests.push_back(copied_root);
        }
    }

    do{
        ll = ChartItem::log_zero;
        t1 = clock();
        for(int i = 0; i < training_size; i++){
            if(i % 50 == 0){
                std::cout << i << "\n";
            }

            for (auto root:forests) {
                addParentPointer(root, generator, 0);
                addRulePointer(root, shrg_rules);
                double pw = computeInside(root);
                computeOutside(root);
                computeExpectedCount(root, pw);

                ll = addLogs(ll, pw);
            }

//             EdsGraph graph = graphs[i];
//             auto code = context->Parse(graph);
//             if(code == ParserError::kNone){
//                 ChartItem *root = context->parser->Result();
// //                TreeInfo info = GetForestInfo(generator, root);
// //                PrintForestInfo(info);
//                 addParentPointer(root, generator, 0);
//                 addRulePointer(root, shrg_rules);
//
//                 double pw = computeInside(root);
//                 computeOutside(root);
//                 computeExpectedCount(root, pw);
//
//                 ll = addLogs(ll, pw);
// //                history_graph_ll[i].push_back(pw);
//             }
        }

        EMUpdate(shrg_rules, rule_dict);
        for(int i = 0; i < shrg_rules.size(); i++){
            history[i].push_back(shrg_rules[i]->log_rule_weight);
        }
        clearRuleCount(shrg_rules);

        t2 = clock();
        double time_diff = (double)(t2 - t1)/CLOCKS_PER_SEC;
        std::cout << "iteration: " << iteration << "\nlog likelihood: " << ll;
        std::cout << ", in " << time_diff << " seconds \n\n";
        iteration++;
//        writeHistoryToFile("./weight_history_nosmooth_allgraphs", history, shrg_rules.size());
//        writeHistoryToFile("./graph_ll_nosmooth_allgraphs", history_graph_ll, training_size);
    }while(ll < threshold);
    writeHistoryToFile("./weight_history_nosmooth_allgraphs", history, shrg_rules.size());
}

// ############################### Debugging Checkers ########################

bool is_negative(double log_prob){
    if(log_prob <= 0){
        return true;
    }
    return false;
}

bool is_parent_computed(ChartItem *parent){
    return parent->outside_visited_status==VISITED;
}

bool is_normal_count(double c){
    if(!isnormal(c) && c != 0.0 && c != ChartItem::log_zero){
        return false;
    }
    return true;
}

}
