#pragma once
#ifndef HELPERS_NS_ALIAS
#define HELPERS_NS_ALIAS H
#endif

#define HELPERS_NS __helpers_namespace
namespace HELPERS_NS {} // create uniq "helpers namespace" for this project
namespace HELPERS_NS_ALIAS = HELPERS_NS; // set your alias for original "helpers namespace" (defined via macro)


#if (_MANAGED == 1) || (_M_CEE == 1)
#define __CLR__
#endif