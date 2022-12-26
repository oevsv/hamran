const app = require('express')();
const express = require('express');

// statically serve /test folder
app.use(express.static('test'));

app.listen(3000, () => {
  console.log('Server started on port 3000');
})