# Input JSON Test Case Catalog

Use [use_case.sh](../use_case.sh) to activate a case quickly:

- List cases: `./use_case.sh --list`
- Activate case: `./use_case.sh case_simple_linear.json`

## Simple cases

- case_simple_linear.json
- case_simple_branching.json
- case_simple_no_failure_match.json

## Edge cases (valid JSON)

- case_edge_missing_optional_fields.json
- case_edge_zero_values.json
- case_edge_zero_mileage.json
- case_edge_high_delay_toll.json
- case_edge_cycle_graph.json
- case_edge_disconnected_components.json
- case_edge_failure_at_source.json
- case_edge_failure_at_sink.json
- case_edge_duplicate_parallel_roads.json
- case_edge_self_loop.json
- case_edge_negative_values.json
- case_edge_empty_roads.json
- case_edge_no_nodes_block.json

## Complex / Big cases

- case_complex_multi_hub.json
- case_big_dense_network.json
- case_big_25_nodes.json

## Invalid input robustness cases

- case_invalid_missing_config.json
- case_invalid_missing_roads.json
- case_invalid_wrong_types.json
