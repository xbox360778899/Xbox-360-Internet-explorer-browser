# Google Search no Xbox 360 — diagnóstico do porte QuickJS

**Status: ainda não funciona.** Este documento registra um teste real de compilação, não um lançamento do motor moderno.

## O que foi testado

- SDK Xbox 360 2010-01, Visual Studio 2010 e compilador PowerPC Xenon, que compilam normalmente o X360Browser atual.
- QuickJSpp (https://github.com/c-smile/quickjspp), porta do QuickJS para Microsoft Visual C++ destinada a VS2017/VS2019.
- Projeto isolado XEX com quickjs.c, libregexp.c, libunicode.c, cutils.c e script ECMAScript moderno de teste (const, arrow functions e Array.map).
- Ajustes experimentais para inttypes.h, fenv.h, windows.h, intrin.h, endianness big-endian do Xbox e inline.

**Resultado:** o protótipo QuickJS NÃO compilou em PowerPC XDK. Após ajustar os primeiros cabeçalhos, a tentativa C gerou pelo menos **102 erros** antes do limite do compilador (declarações C99 e incompatibilidades de tipo). A tentativa C++ gerou pelo menos **126 erros** (conversões inválidas de void*, _BitScan*, _msize e outras APIs ausentes). Os números são parciais. QuickJSpp oficialmente descreve suporte a Visual Studio 2017/2019, não ao compilador PowerPC do Xbox SDK 2010.

Por isso, nenhuma parte dessa tentativa quebrada foi ligada ao default.xex de produção. Duktape continua sendo o motor ativo; não houve substituição validada.

## Teste verificável do Google

Execute na raiz do projeto:

    python tools/verify_google_results.py

Em 01/11/2026, a resposta direta de https://www.google.com/search?q=Xbox%20360 foi HTTP 200 com aproximadamente 93 KB, mas continha a exigência de JavaScript em /httpservice/retry/enablejs e nenhum resultado orgânico real. O teste exibiu FAIL. Com identificação de navegador Lynx, o Google respondeu com pedido de atualização do navegador, também sem resultados. Trocar o User-Agent não é solução.

## Para que o Google realmente funcione

1. Compilador moderno com ABI Xenon/XDK correto, código PowerPC big-endian e ambiente C/CRT compatível, seguido de porte de runtime ES2020+.
2. XEX mínimo com o novo motor, executado em Xbox RGH/JTAG real, com limites de memória e execução.
3. DOM persistente, window/document, eventos, CSSOM, Promise e execução de módulos, em vez do preprocessamento HTML atual.
4. Web APIs seguras: fetch/XHR, URL, cookies, storage, regras de origem e TLS.
5. Renderizador HTML/CSS atualizado para Flexbox, Grid e layout web moderno.
6. Teste final obrigatório: resultados com títulos e links verdadeiramente obtidos do Google, renderizados e clicáveis no Xbox físico. Bing, imagens estáticas e navegadores remotos não contam.

O compilador LLVM com suporte ao ABI Xenon do OXDK é um possível caminho de pesquisa, mas não foi instalado nem validado neste computador. O compilador PowerPC genérico do LLVM não substitui automaticamente o ABI do XDK.

**Até passar no teste acima, a pesquisa Google continua não implementada.**
