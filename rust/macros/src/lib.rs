use std::{io::Write, path::Path};

use proc_macro::{Literal, TokenStream};
use quote::quote;
use syn::{parse_macro_input, ItemEnum, LitStr, Meta, Type};

#[proc_macro_attribute]
pub fn generate_lua_enum(args: TokenStream, input: TokenStream) -> TokenStream {
    // Parse the input token stream into a syn ItemEnum (an enum definition)
    let input = parse_macro_input!(input as ItemEnum);
    let mut repr_type: Option<Type> = None;
    let mut lua_path: Option<LitStr> = None;

    let args_parser = syn::meta::parser(|meta| {
        if meta.path.is_ident("repr") {
            repr_type = Some(meta.value()?.parse()?);
            Ok(())
        } else if meta.path.is_ident("lua_path") {
            lua_path = Some(meta.value()?.parse()?);
            Ok(())
        } else {
            Err(meta.error("unsupported property"))
        }
    });

    parse_macro_input!(args with args_parser);

    let repr_type = repr_type.unwrap();
    let lua_path_str = lua_path.unwrap().value().to_owned();
    let lua_path = Path::new(&lua_path_str);

    // Extract the enum name
    let enum_name = &input.ident;

    // Generate Lua code based on the enum variants
    let mut lua_code = format!("{} = {{\n", enum_name);
    let mut lua_reverse = format!("{}Reverse = {{\n", enum_name);

    let mut variants = vec![];
    let mut try_from_mappings = vec![];
    let mut from_repr_type_mappings = vec![];

    for variant in &input.variants {
        let variant_name = &variant.ident;
        let discriminant = if let Some((_, expr)) = &variant.discriminant {
            quote! { #expr }
        } else {
            panic!("All enum variants must have a discriminant (explicit value)");
        };
        variants.push(quote! { #variant_name = #discriminant, });
        try_from_mappings.push(quote! { #discriminant => Ok(#enum_name::#variant_name), });
        from_repr_type_mappings.push(quote! { #enum_name::#variant_name => #discriminant, });
        lua_code.push_str(&format!("    {} = {},\n", variant_name, discriminant));
        lua_reverse.push_str(&format!(
            "    [{}.{}] = '{}',\n",
            enum_name, variant_name, variant_name
        ));
    }

    lua_code.push_str("}\n");
    lua_reverse.push_str("}\n");

    // Write the Lua code to a file
    // let lua_path = format!("generated/{}.lua", enum_name.to_string().to_lowercase());
    let mut file = std::fs::File::create(&lua_path).unwrap();
    write!(file, "{}", lua_code).expect("Unable to write Lua code");
    write!(file, "\n\n").expect("Unable to write Lua code");
    write!(file, "{}", lua_reverse).expect("Unable to write Lua code");

    // Generate the Rust code for the enum
    let expanded = quote! {
        #[repr(#repr_type)]
        pub enum #enum_name {
            #(#variants)*
        }

        impl TryFrom<#repr_type> for #enum_name {
            type Error = LoleError;
            fn try_from(value: #repr_type) -> std::result::Result<Self, Self::Error> {
                match value {
                    #(#try_from_mappings)*
                    v => Err(LoleError::InvalidEnumValue(format!(concat!(stringify!($name), "::try_from({})"), v)))
                }
            }
        }

        impl From<#enum_name> for #repr_type {
            fn from(value: #enum_name) -> Self {
                match value {
                    #(#from_repr_type_mappings)*
                }
            }
        }
    };

    // Return the generated Rust code as a TokenStream
    TokenStream::from(expanded)
}
