# XamlRuntime package sources

`XamlRuntime` builds the platform-specific static UI runtime.
`XamlCompiler` builds the Windows host tool that compiles `.xaml` pages.

They are packaged together, but never built by the same target: consumers link
`XamlRuntime`, while build integration invokes `XamlCompiler` before compiling
generated page sources.