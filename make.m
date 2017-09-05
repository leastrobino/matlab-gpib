%
%  make.m
%
%  Created by Léa Strobino.
%  Copyright 2017 hepia. All rights reserved.
%

function make(varargin)

MEX = {};
switch computer('arch')
  case 'win64'
    MEX = [MEX {'-largeArrayDims'}];
    MEX = [MEX {'-outdir','private'}];
  otherwise
    error('GPIBlpt:make','%s is not supported.',upper(computer('arch')));
end

if nargin == 0
  varargin = {'clean','all'};
end

warning('off','MATLAB:DELETE:FileNotFound');

if any(strcmpi(varargin,'clean'))
  m = mexext('all');
  for i = 1:length(m)
    delete(['*.' m(i).ext]);
    delete(['private/*.' m(i).ext]);
  end
  delete *.p
  delete private/*.p
end

if any(strcmpi(varargin,'all')) || any(strcmpi(varargin,'gpiblpt'))
  mex('gpiblpt.c',MEX{:});
end

if any(strcmpi(varargin,'all')) || any(strcmpi(varargin,'manage_driver'))
  mex('manage_driver.c',MEX{:},'-outdir','.');
end

if any(strcmpi(varargin,'all')) || any(strcmpi(varargin,'p'))
  pcode GPIBlpt.m
end
