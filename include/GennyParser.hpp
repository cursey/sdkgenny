#pragma once

#include <tao/pegtl.hpp>

#include "Genny.hpp"

namespace genny::parser {
    using namespace tao::pegtl;

    struct Comment : disable<one<'#'>, until<eolf>> {};
    struct Sep : sor<one<' ', '\t'>, Comment> {};
    struct Seps : star<Sep> {};

    struct HexNum : seq<one<'0'>, one<'x'>, plus<xdigit>> {};
    struct DecNum: plus<digit> {};
    struct Num : sor<HexNum, DecNum> {};

    struct NsId : TAO_PEGTL_STRING("namespace") {};
    struct NsName : identifier {};
    struct NsNameList : list<NsName, one<'.'>, Sep> {};
    struct NsDecl : seq<NsId, Seps, opt<NsNameList>> {};

    struct TypeId: TAO_PEGTL_STRING("type") {};
    struct TypeName : identifier {};
    struct TypeSize : Num {};
    struct TypeDecl : seq<TypeId, Seps, TypeName, Seps, TypeSize> {};

    struct StructId : TAO_PEGTL_STRING("struct") {};
    struct StructName : identifier {};
    struct StructParent : identifier {};
    struct StructParentList : list<StructParent, one<','>, Sep> {};
    struct StructParentListDecl : seq<one<':'>, Seps, StructParentList> {}; 
    struct StructDecl : seq<StructId, Seps, StructName, Seps, opt<StructParentListDecl>> {};

    struct VarType : identifier {};
    struct VarName : identifier {};
    struct VarOffset: Num {};
    struct VarOffsetDecl : seq<one<'@'>, Seps, VarOffset> {};
    struct VarDecl : seq<VarType, Seps, VarName, Seps, opt<VarOffsetDecl>> {};

    struct Decl : seq<Seps, sor<NsDecl, TypeDecl, StructDecl, VarDecl>, Seps> {};
    struct Grammar : until<eof, sor<eolf, Decl>> {};
}
