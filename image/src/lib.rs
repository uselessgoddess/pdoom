#![feature(proc_macro_span)]

use {
    proc_macro::{Span, TokenStream},
    proc_macro2::Ident,
    quote::{quote, TokenStreamExt},
    syn::{
        parse::{self, ParseStream},
        parse_macro_input, LitStr, Token,
    },
};

struct Input {
    path: LitStr,
    in_kw: Token![in],
    name: Ident,
}

impl parse::Parse for Input {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        Ok(Input { path: input.parse()?, in_kw: input.parse()?, name: input.parse()? })
    }
}

#[proc_macro]
pub fn load(input: TokenStream) -> TokenStream {
    let cwf = Span::call_site().source_file().path();
    let cwd = cwf.parent().unwrap();

    let Input { path, name, .. } = parse_macro_input!(input as Input);
    let image = image::open(cwd.join(path.value())).unwrap().to_rgb8();

    let mut repr = quote! {};
    for image::Rgb([r, g, b]) in image.pixels() {
        repr.append_all(quote!(#r, #g, #b, ));
    }

    let len = image.pixels().count();
    let (w, h) = image.dimensions();
    quote!(static #name: ([u8; #len * 3], usize, usize) = ([#repr], #w as usize, #h as usize);)
        .into()
}
