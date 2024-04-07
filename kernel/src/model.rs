extern crate alloc;

use alloc::vec::Vec;

pub struct Vertex {
    position: [f32; 3],
}

pub struct Triangle {
    vertices: [Vertex; 3],
}

#[repr(C)]
struct ObjRepr {
    triangles: *mut Triangle,
    triangles_len: usize,
}

macro_rules! define_model {
    ($path:literal => $name:ident) => {
        const _: () = {
            const OBJ: &str = include_str!($path);

            #[no_mangle]
            extern "C" fn $name() -> ObjRepr {
                let obj = wavefront::Obj::from_lines(OBJ.lines()).unwrap();

                let repr = obj
                    .triangles()
                    .map(|t| Triangle { vertices: t.map(|v| Vertex { position: v.position() }) })
                    .collect::<Vec<_>>();

                let (triangles, triangles_len, _) = repr.into_raw_parts();
                ObjRepr { triangles, triangles_len }
            }
        };
    };
}

define_model!("../assets/niger.obj" => load_elemental);
