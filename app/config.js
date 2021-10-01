
let d = document;
let p = p => fetch(p).then(async resp => d.getElementById('page').innerHTML = await resp.text());
let e = (t, i, c) => d.addEventListener(t, (e) => { if (e.target.id == i) { e.preventDefault(); c(e);} }, false);

p('login');

e('click', 'login', e => {
    console.log('event', e);
})
