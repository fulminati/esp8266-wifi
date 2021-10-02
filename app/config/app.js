
p('connect');

e('click', 'connect', e => {
    console.log('event', e);
})

e('click', 'test', e => {
    o('show')
    setTimeout(() => {
        o('hide')
    }, 3000)
})
