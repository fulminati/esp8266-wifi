let d = document;
let g = i => d.getElementById(i)
let p = (p, f) => fetch(p, {method: 'POST', body: f ? new FormData(g(f)) : '' }).then(async resp => g('page').innerHTML = await resp.text());
let e = (t, i, c) => d.addEventListener(t, (e) => { if (e.target.id == i) { e.preventDefault(); c(e.target);} }, false);
let o = a => g('mask').style.display = a == 'show' ? 'block' : 'none';
let r = u => { window.location.href = u };
