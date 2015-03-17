/* https://bitbucket.org/birkenfeld/sphinx/issue/969/allow-mathjax-customization-via-localjs  */
document.write('\
<script type="text/x-mathjax-config">\
  MathJax.Hub.Config({\
    TeX: {\
      Macros: {\
        prev:["#1^-",1],pprev:["#1^{\\\\ominus}",1],curr:["#1^{\\\\circ}",1],nnext:["#1^{\\\\oplus}",1],next:["#1^{+}",1],Dt:"\\\\Delta t",tens:["\\\\boldsymbol{#1}",1],vec:["\\\\boldsymbol{#1}",1],mat:["\\\\boldsymbol{#1}",1],u:["\\\\,\\\\mathrm{#1}",1],normalized:["\\\\frac{#1}{|\\\\cdot|}",1],d:"\\\\mathrm{d}\\\\,",phi:"\\\\varphi",eps:"\\\\varepsilon",epsilon:"\\\\varepsilon",tr:"\\\\mathop{\\\\rm tr}\\\\nolimits" \
      }\
    },\
	 "HTML-CSS": {availableFonts:"TeX",webFont:"TeX"} \
  });\
</script>')
document.write('<script type="text/javascript" src="https://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS_HTML"></script>');
