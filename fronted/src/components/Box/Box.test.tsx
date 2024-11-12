import React from 'react';
import ReactDOM from 'react-dom';
import Box from './Box';

it('It should mount', () => {
  const div = document.createElement('div');
  ReactDOM.render(<Box />, div);
  ReactDOM.unmountComponentAtNode(div);
});