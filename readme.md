All parts of the code relevant to the inside-outside algorithm are in the file src/python/inside_outside.cpp



src/generate.cpp contains the main method that prepares and runs the algorithm. It takes 3 arguments:

1. parser type: linear or tree
2. grammar_path
3. graph_path

For example:

linear \ data/eds.construction.no_IS.delex.no_punct/train.mapping.txt data/eds.construction.no_IS.delex.no_punct/train.graph.txt

