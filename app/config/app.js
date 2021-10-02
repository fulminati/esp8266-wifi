
p('config').then(() => {


});

e('click', 'connect', e => {
    o('show')
    p('connect').then(() => {

        o('hide')
    });
})

e('click', 'test', e => {
    o('show')
    setTimeout(() => {
        o('hide')
    }, 3000)
})
