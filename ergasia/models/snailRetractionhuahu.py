import sys

def filter_snail_obj(input_filename, output_filename):
    vertices = []
    # Map from old_vertex_index (1-based) to new_vertex_index (1-based)
    vertex_map = {} 
    
    kept_vertices = []
    kept_faces = []
    
    # Texture coords (vt) and Normals (vn) are kept as-is to avoid complex re-indexing.
    # Unused vt/vn lines do not break OBJ files.
    other_lines = [] 

    # --- 1. FILTERING LOGIC ---
    # Adjust these thresholds if the cut isn't perfect
    FOOT_Y_LIMIT = 0.35       # Remove anything below this height
    HEAD_Z_LIMIT = 1.4       # Remove anything extending past this Z...
    SHELL_SAFE_HEIGHT = 0.8   # ...UNLESS it is high up (part of the shell)

    print(f"Processing {input_filename}...")
    
    try:
        with open(input_filename, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: Could not find {input_filename}")
        return

    # Pass 1: Read and Filter Vertices
    v_counter = 0
    new_v_counter = 0
    
    for line in lines:
        if line.startswith('v '):
            v_counter += 1
            parts = line.strip().split()
            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
            
            # LOGIC: Keep vertex if it is NOT (Foot OR Protruding Head)
            # 1. Is it the foot? (Low Y)
            is_foot = y < FOOT_Y_LIMIT
            
            # 2. Is it the head/eyes? (Forward Z AND Low Y)
            # We check y < SHELL_SAFE_HEIGHT to avoid deleting the back of the shell
            is_head = (z < HEAD_Z_LIMIT) and (y < SHELL_SAFE_HEIGHT)
            
            if not is_foot and not is_head:
                # Keep this vertex
                new_v_counter += 1
                vertex_map[v_counter] = new_v_counter
                kept_vertices.append(line)
            else:
                # Discard vertex (map to None)
                vertex_map[v_counter] = None
                
        elif line.startswith('f '):
            # We will process faces in Pass 2
            pass
        else:
            # Keep comments, groups, vt, vn, usemtl, s, etc.
            other_lines.append(line)

    # Pass 2: Rebuild Faces
    for line in lines:
        if line.startswith('f '):
            parts = line.strip().split()
            # Face format: f v1/vt1/vn1 v2/vt2/vn2 ...
            # We only care about the first number (v index)
            
            new_face_parts = ["f"]
            is_valid_face = True
            
            for p in parts[1:]:
                indices = p.split('/')
                v_idx = int(indices[0])
                
                # Check if this vertex was kept
                if v_idx in vertex_map and vertex_map[v_idx] is not None:
                    # Update the vertex index to the new one
                    new_v_idx = str(vertex_map[v_idx])
                    
                    # Reconstruct the "v/vt/vn" string with the new v index
                    # We keep vt and vn indices unchanged
                    new_component = new_v_idx + (line[line.find('/'):] if '/' in p else "")
                    # A safer way to reconstruct keeping suffix:
                    suffix = p[len(str(v_idx)):] 
                    new_face_parts.append(f"{new_v_idx}{suffix}")
                else:
                    # If any vertex in the face was removed, destroy the whole face
                    is_valid_face = False
                    break
            
            if is_valid_face:
                kept_faces.append(" ".join(new_face_parts) + "\n")

    # --- WRITE OUTPUT ---
    with open(output_filename, 'w') as f:
        f.write("# Generated Retracted Snail Mesh\n")
        # Write vertices first
        f.writelines(kept_vertices)
        # Write other data (vt, vn, materials)
        f.writelines(other_lines)
        # Write updated faces
        f.writelines(kept_faces)

    print(f"Success! Created {output_filename}")
    print(f"Original Vertices: {v_counter}")
    print(f"Retracted Vertices: {new_v_counter}")

if __name__ == "__main__":
    filter_snail_obj("Mesh_Snail.obj", "Mesh_Snail_Retracted.obj")