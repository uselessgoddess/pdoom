extern crate alloc;

use alloc::vec::Vec;

pub struct Vertex {
    position: [f32; 3],
}

#[repr(C)]
pub struct Triangle {
    vertices: [Vertex; 3],
    uv: [[f32; 3]; 3],
}

#[repr(C)]
struct Texture {
    ptr: *const u8,
    len: usize,
    w: usize,
    h: usize,
}

#[repr(C)]
struct ObjRepr {
    triangles: *mut Triangle,
    triangles_len: usize,
    texture: Texture,
}

#[rustfmt::skip]
macro_rules! define_model {
    (obj: $obj:literal texture: $image:literal => $name:ident) => {
        const _: () = {
            image::load!($image in IMG);
            
            static OBJ: &str = include_str!($obj);
            
            #[no_mangle]
            extern "C" fn $name() -> ObjRepr {
                let obj = wavefront::Obj::from_lines(OBJ.lines()).unwrap();

                let repr = obj
                    .triangles()
                    .map(|t| Triangle {
                        vertices: t.map(|v| Vertex { position: v.position() }),
                        uv: t.map(|v| v.uv().unwrap()),
                    })
                    .collect::<Vec<_>>();

                let (triangles, triangles_len, _) = repr.into_raw_parts();
                let (img, w, h) = &IMG;
                ObjRepr { 
                    triangles,
                    triangles_len, 
                    texture: Texture {
                        ptr: img.as_ptr(),
                        len: img.len(),
                        w: *w, h: *h,
                    } 
                }
            }
        };
    };
}

define_model!(
    obj: "../assets/niger.obj"
    texture: "../assets/niger.tga"
    => load_elemental
);
