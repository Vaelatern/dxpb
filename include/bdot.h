/*
 * bdot.h
 *
 * Wrapper for working with graphviz
 */

Agraph_t	*bdot_from_graph(bgraph);
Agraph_t	*bdot_toplevel_from_graph(bgraph);
Agraph_t	*bdot_from_graph_for_arch(bgraph, const char *);
Agraph_t	*bdot_toplevel_from_graph_for_arch(bgraph, const char *);
void		 bdot_save_graph(Agraph_t *, const char *);
void		 bdot_print_graph(Agraph_t *grph);
void		 bdot_close(Agraph_t **);
