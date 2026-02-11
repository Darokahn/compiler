#!/usr/bin/env python3
"""
CSV to Flow Diagram Converter

This program reads a CSV file representing a state transition table and generates
a flow diagram visualization using Graphviz.

CSV Format:
- First row: header with state names (first column empty, rest are state names)
- First column: character types/input symbols
- Cells: next state given current state (column) and input character (row)

Example usage:
    python csv_to_flow_diagram.py input.csv output.png
    python csv_to_flow_diagram.py input.csv output.pdf --format pdf
"""

import csv
import argparse
import sys
from pathlib import Path

try:
    from graphviz import Digraph
except ImportError:
    print("Error: graphviz package not found.")
    print("Please install it using: pip install graphviz --break-system-packages")
    print("Note: You also need Graphviz system package installed.")
    print("  Ubuntu/Debian: sudo apt-get install graphviz")
    print("  macOS: brew install graphviz")
    sys.exit(1)


class StateTransitionTable:
    """Represents a state transition table from a CSV file."""
    
    def __init__(self, csv_file):
        """
        Initialize the state transition table from a CSV file.
        
        Args:
            csv_file: Path to the CSV file
        """
        self.states = []
        self.characters = []
        self.transitions = {}
        self._load_csv(csv_file)
    
    def _load_csv(self, csv_file):
        """Load and parse the CSV file."""
        with open(csv_file, 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            
            # Read header row (states)
            header = next(reader)
            self.states = [state.strip() for state in header[1:] if state.strip()]
            
            # Read data rows (character types and transitions)
            for row in reader:
                if not row or not row[0].strip():
                    continue
                
                char_type = row[0].strip()
                self.characters.append(char_type)
                
                # Process transitions for this character type
                for col_idx, next_state in enumerate(row[1:], start=0):
                    if col_idx >= len(self.states):
                        break
                    
                    current_state = self.states[col_idx]
                    next_state = next_state.strip()
                    
                    # Empty cells mean transition to CANTMOVESTATE
                    if not next_state:
                        next_state = 'CANTMOVESTATE'
                    
                    key = (current_state, char_type)
                    self.transitions[key] = next_state
    
    def get_unique_transitions(self):
        """
        Get all unique state-to-state transitions with their triggering characters.
        Uses "all valid others" for edges where the majority of character types lead to the same destination.
        
        Returns:
            dict: {(from_state, to_state): [list of character types] or "all valid others"}
        """
        edge_labels = {}
        
        for (from_state, char_type), to_state in self.transitions.items():
            edge = (from_state, to_state)
            if edge not in edge_labels:
                edge_labels[edge] = []
            edge_labels[edge].append(char_type)
        
        # For each source state, find if there's a majority destination
        state_transitions = {}
        for (from_state, char_type), to_state in self.transitions.items():
            if from_state not in state_transitions:
                state_transitions[from_state] = {}
            if to_state not in state_transitions[from_state]:
                state_transitions[from_state][to_state] = []
            state_transitions[from_state][to_state].append(char_type)
        
        # Determine if we should use "all valid others" for any edges
        optimized_labels = {}
        for from_state, destinations in state_transitions.items():
            total_chars = sum(len(chars) for chars in destinations.values())
            
            # Find the destination with the most character types
            max_dest = max(destinations.items(), key=lambda x: len(x[1]))
            max_dest_state, max_dest_chars = max_dest
            
            # If this destination has more than half of all transitions, use "all valid others"
            if len(max_dest_chars) > total_chars / 2:
                # This is the majority destination - it gets "all valid others"
                for to_state, chars in destinations.items():
                    edge = (from_state, to_state)
                    if to_state == max_dest_state:
                        optimized_labels[edge] = "all valid others"
                    else:
                        optimized_labels[edge] = chars
            else:
                # No majority, use original labels
                for to_state, chars in destinations.items():
                    edge = (from_state, to_state)
                    optimized_labels[edge] = chars
        
        return optimized_labels
    
    def create_flow_diagram(self, output_file, format='png', engine='dot'):
        """
        Create a flow diagram from the state transition table.
        
        Args:
            output_file: Path for the output file (without extension)
            format: Output format (png, pdf, svg, etc.)
            engine: Graphviz layout engine (dot, neato, fdp, sfdp, circo, twopi)
        """
        # Create a new directed graph
        dot = Digraph(comment='State Transition Diagram', engine=engine)
        dot.attr(rankdir='LR')  # Left to right layout
        
        # Graph aesthetics
        dot.attr('node', shape='circle', style='filled', fillcolor='lightblue', 
                 fontname='Arial', fontsize='12')
        dot.attr('edge', fontname='Arial', fontsize='10')
        
        # Add special formatting for start and end states (but not CANTMOVESTATE)
        for state in self.states:
            if state == 'CANTMOVESTATE':
                # Skip - we'll handle this specially
                continue
            elif 'START' in state.upper():
                dot.node(state, state, shape='circle', fillcolor='lightgreen', 
                        penwidth='2.0')
            elif state.upper() == "EOFSTATE":
                dot.node(state, state, shape='doublecircle', fillcolor='lightcoral')
            else:
                dot.node(state, state)
        
        # Get unique transitions and group by edge
        edge_labels = self.get_unique_transitions()
        
        # Counter for creating unique error nodes
        error_node_counter = 0
        
        # Add edges with labels
        for (from_state, to_state), char_types in edge_labels.items():
            # Create a label from character types
            if char_types == "all valid others":
                label = "all valid others"
            elif len(char_types) <= 3:
                label = '\\n'.join(char_types)
            else:
                # If too many, show first few and count
                label = '\\n'.join(char_types[:3]) + f'\\n(+{len(char_types)-3} more)'
            
            # Special handling for CANTMOVESTATE transitions
            if to_state == 'CANTMOVESTATE':
                # Create a unique small yellow error node for this edge
                error_node_name = f'error_{error_node_counter}'
                error_node_counter += 1
                dot.node(error_node_name, shape='circle', fillcolor='yellow', 
                        width='0.3', height='0.3', fixedsize='true', label='')
                dot.edge(from_state, error_node_name, label=label)
            # Add self-loops with special styling
            elif from_state == to_state:
                dot.edge(from_state, to_state, label=label, color='blue')
            else:
                dot.edge(from_state, to_state, label=label)
        
        # Render the diagram
        try:
            output_path = Path(output_file)
            output_base = str(output_path.with_suffix(''))
            dot.render(output_base, format=format, cleanup=True)
            print(f"Flow diagram created successfully: {output_base}.{format}")
            return f"{output_base}.{format}"
        except Exception as e:
            print(f"Error rendering diagram: {e}")
            sys.exit(1)


def main():
    """Main function to handle command line arguments and create the diagram."""
    parser = argparse.ArgumentParser(
        description='Convert CSV state transition table to flow diagram',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s transitions.csv output.png
  %(prog)s transitions.csv output.pdf --format pdf
  %(prog)s transitions.csv diagram.svg --format svg --engine neato

Supported formats: png, pdf, svg, ps, eps, and more
Supported engines: dot (default), neato, fdp, sfdp, circo, twopi
        """
    )
    
    parser.add_argument('input_csv', help='Input CSV file containing state transition table')
    parser.add_argument('output_file', help='Output file path (extension will be added based on format)')
    parser.add_argument('--format', '-f', default='png', 
                       help='Output format (default: png). Options: png, pdf, svg, ps, eps, etc.')
    parser.add_argument('--engine', '-e', default='dot',
                       choices=['dot', 'neato', 'fdp', 'sfdp', 'circo', 'twopi'],
                       help='Graphviz layout engine (default: dot)')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Print detailed information')
    
    args = parser.parse_args()
    
    # Check if input file exists
    if not Path(args.input_csv).exists():
        print(f"Error: Input file '{args.input_csv}' not found.")
        sys.exit(1)
    
    # Load the state transition table
    if args.verbose:
        print(f"Loading state transition table from: {args.input_csv}")
    
    table = StateTransitionTable(args.input_csv)
    
    if args.verbose:
        print(f"Found {len(table.states)} states: {', '.join(table.states)}")
        print(f"Found {len(table.characters)} character types")
        print(f"Found {len(table.transitions)} transitions")
    
    # Create the flow diagram
    if args.verbose:
        print(f"Creating flow diagram using '{args.engine}' engine...")
    
    output_path = table.create_flow_diagram(args.output_file, args.format, args.engine)
    
    if args.verbose:
        print("Done!")
    
    return output_path


if __name__ == '__main__':
    main()
