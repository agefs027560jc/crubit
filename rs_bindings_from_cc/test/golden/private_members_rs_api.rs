// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Automatically @generated Rust bindings for C++ target
// //rs_bindings_from_cc/test/golden:private_members_cc
#![rustfmt::skip]
#![feature(const_ptr_offset_from, custom_inner_attributes)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![deny(warnings)]

use ::std as rust_std;

// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

pub mod test_namespace_bindings {
    #[derive(Clone, Copy)]
    #[repr(C, align(4))]
    pub struct SomeClass {
        __non_field_data: [crate::rust_std::mem::MaybeUninit<u8>; 0],
        pub public_member_variable_: i32,
        /// Reason for representing this field as a blob of bytes:
        /// Types of non-public C++ fields can be elided away
        pub(crate) private_member_variable_: [crate::rust_std::mem::MaybeUninit<u8>; 4],
    }
    forward_declare::unsafe_define!(
        forward_declare::symbol!("SomeClass"),
        crate::test_namespace_bindings::SomeClass
    );

    impl Default for SomeClass {
        #[inline(always)]
        fn default() -> Self {
            let mut tmp = crate::rust_std::mem::MaybeUninit::<Self>::zeroed();
            unsafe {
                crate::detail::__rust_thunk___ZN23test_namespace_bindings9SomeClassC1Ev(&mut tmp);
                tmp.assume_init()
            }
        }
    }

    impl<'b> From<ctor::RvalueReference<'b, crate::test_namespace_bindings::SomeClass>> for SomeClass {
        #[inline(always)]
        fn from(
            __param_0: ctor::RvalueReference<'b, crate::test_namespace_bindings::SomeClass>,
        ) -> Self {
            let mut tmp = crate::rust_std::mem::MaybeUninit::<Self>::zeroed();
            unsafe {
                crate::detail::__rust_thunk___ZN23test_namespace_bindings9SomeClassC1EOS0_(
                    &mut tmp, __param_0,
                );
                tmp.assume_init()
            }
        }
    }

    // rs_bindings_from_cc/test/golden/private_members.h;l=11
    // Error while generating bindings for item 'SomeClass::operator=':
    // Bindings for this kind of operator are not supported

    // rs_bindings_from_cc/test/golden/private_members.h;l=11
    // Error while generating bindings for item 'SomeClass::operator=':
    // Bindings for this kind of operator are not supported

    impl SomeClass {
        #[inline(always)]
        pub fn public_method<'a>(&'a mut self) {
            unsafe {
                crate::detail::__rust_thunk___ZN23test_namespace_bindings9SomeClass13public_methodEv(
                    self,
                )
            }
        }
    }

    impl SomeClass {
        #[inline(always)]
        pub fn public_static_method() {
            unsafe {
                crate::detail::__rust_thunk___ZN23test_namespace_bindings9SomeClass20public_static_methodEv()
            }
        }
    }
}

// namespace test_namespace_bindings

// CRUBIT_RS_BINDINGS_FROM_CC_TEST_GOLDEN_PRIVATE_MEMBERS_H_

mod detail {
    #[allow(unused_imports)]
    use super::*;
    extern "C" {
        pub(crate) fn __rust_thunk___ZN23test_namespace_bindings9SomeClassC1Ev<'a>(
            __this: &'a mut crate::rust_std::mem::MaybeUninit<
                crate::test_namespace_bindings::SomeClass,
            >,
        );
        pub(crate) fn __rust_thunk___ZN23test_namespace_bindings9SomeClassC1EOS0_<'a, 'b>(
            __this: &'a mut crate::rust_std::mem::MaybeUninit<
                crate::test_namespace_bindings::SomeClass,
            >,
            __param_0: ctor::RvalueReference<'b, crate::test_namespace_bindings::SomeClass>,
        );
        #[link_name = "_ZN23test_namespace_bindings9SomeClass13public_methodEv"]
        pub(crate) fn __rust_thunk___ZN23test_namespace_bindings9SomeClass13public_methodEv<'a>(
            __this: &'a mut crate::test_namespace_bindings::SomeClass,
        );
        #[link_name = "_ZN23test_namespace_bindings9SomeClass20public_static_methodEv"]
        pub(crate) fn __rust_thunk___ZN23test_namespace_bindings9SomeClass20public_static_methodEv();
    }
}

const _: () = assert!(rust_std::mem::size_of::<Option<&i32>>() == rust_std::mem::size_of::<&i32>());

const _: () = assert!(rust_std::mem::size_of::<crate::test_namespace_bindings::SomeClass>() == 8);
const _: () = assert!(rust_std::mem::align_of::<crate::test_namespace_bindings::SomeClass>() == 4);
const _: () = {
    static_assertions::assert_impl_all!(crate::test_namespace_bindings::SomeClass: Clone);
};
const _: () = {
    static_assertions::assert_impl_all!(crate::test_namespace_bindings::SomeClass: Copy);
};
const _: () = {
    static_assertions::assert_not_impl_all!(crate::test_namespace_bindings::SomeClass: Drop);
};
const _: () = assert!(
    memoffset_unstable_const::offset_of!(
        crate::test_namespace_bindings::SomeClass,
        public_member_variable_
    ) == 0
);
const _: () = assert!(
    memoffset_unstable_const::offset_of!(
        crate::test_namespace_bindings::SomeClass,
        private_member_variable_
    ) == 4
);
