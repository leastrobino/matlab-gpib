%
%  GPIBlpt.m
%
%  Created by Léa Strobino.
%  Copyright 2017 hepia. All rights reserved.
%

classdef GPIBlpt < matlab.mixin.SetGet
  
  properties (SetAccess = private)
    PrimaryAddress
  end
  
  properties
    EOIMode = 'read';
    EOSMode = 'write';
    EOSCharCode = uint8(10);
    InputBufferSize = uint32(2^20);
    Timeout = 200E-3;
  end
  
  properties (SetAccess = private)
    ServiceRequest
  end
  
  properties (Access = private) %#ok<*MCSUP>
    useEOI = true;
    useEOSread = false;
    useEOSwrite = true;
    timeout = uint16(200);
  end
  
  methods
    
    function this = GPIBlpt(PrimaryAddress,varargin)
      if PrimaryAddress < 1 || PrimaryAddress > 31
        error('GPIBlpt:PrimaryAddressOutOfRange',...
          'Parameter out of range: PrimaryAddress (1-31).');
      end
      this.PrimaryAddress = uint8(PrimaryAddress);
      if nargin > 1
        this.set(varargin{:});
      end
    end
    
    function e = eq(this,o)
      e = (this.PrimaryAddress == o.PrimaryAddress);
    end
    
    function this = set.EOIMode(this,EOIMode)
      switch lower(EOIMode)
        case 'none'
          this.EOIMode = 'none';
          this.useEOI = false;
        case 'read'
          this.EOIMode = 'read';
          this.useEOI = true;
        otherwise
          error('GPIBlpt:EOIModeInvalid',...
            '''%s'' is not a valid value.\nUse one of these values: ''none'' | ''read''.',...
            EOIMode);
      end
    end
    
    function this = set.EOSCharCode(this,EOSCharCode)
      this.EOSCharCode = uint8(EOSCharCode(1));
    end
    
    function this = set.EOSMode(this,EOSMode)
      switch lower(EOSMode)
        case 'none'
          this.EOSMode = 'none';
          this.useEOSread = false;
          this.useEOSwrite = false;
        case 'read'
          this.EOSMode = 'read';
          this.useEOSread = true;
          this.useEOSwrite = false;
        case 'write'
          this.EOSMode = 'write';
          this.useEOSread = false;
          this.useEOSwrite = true;
        case 'read&write'
          this.EOSMode = 'read&write';
          this.useEOSread = true;
          this.useEOSwrite = true;
        otherwise
          error('GPIBlpt:EOSModeInvalid',...
            '''%s'' is not a valid value.\nUse one of these values: ''none'' | ''read'' | ''write'' | ''read&write''.',...
            EOSMode);
      end
    end
    
    function this = set.InputBufferSize(this,InputBufferSize)
      this.InputBufferSize = uint32(InputBufferSize(1));
    end
    
    function this = set.Timeout(this,Timeout)
      this.Timeout = max([1E-3,min([60,Timeout])]);
      this.timeout = uint16(1E3*this.Timeout);
    end
    
    function d = fread(this,length)
      if nargin < 2
        length = this.InputBufferSize;
      end
      try
        d = gpiblpt('readString',...
          this.PrimaryAddress,...
          uint32(length),...
          this.useEOI,...
          this.useEOSread,...
          this.EOSCharCode,...
          this.timeout);
      catch e
        this.error(e);
      end
    end
    
    function [d,count,msg] = fscanf(this,format,varargin)
      if nargin < 2
        format = '%c';
      end
      try
        d = gpiblpt('readString',...
          this.PrimaryAddress,...
          this.InputBufferSize,...
          this.useEOI,...
          this.useEOSread,...
          this.EOSCharCode,...
          this.timeout);
      catch e
        this.error(e);
      end
      if this.useEOSread
        d(d == this.EOSCharCode) = 10;
      end
      [d,count,msg] = sscanf(char(d),format,varargin{:});
    end
    
    function fwrite(this,d)
      try
        gpiblpt('writeString',...
          [this.PrimaryAddress],...
          uint8(d),...
          max([this.timeout]));
      catch e
        this.error(e);
      end
    end
    
    function fprintf(this,format,varargin)
      if nargin > 2
        s = sprintf(format,varargin{:});
      else
        s = [sprintf(format) 10];
      end
      if this(1).useEOSwrite
        s(s == 10) = this(1).EOSCharCode;
      end
      try
        gpiblpt('writeString',...
          [this.PrimaryAddress],...
          uint8(s),...
          max([this.timeout]));
      catch e
        this.error(e);
      end
    end
    
    function gotolocal(this)
      try
        gpiblpt('goToLocal',...
          [this.PrimaryAddress],...
          max([this.timeout]));
      catch e
        this.error(e);
      end
    end
    
    function clrdevice(this)
      try
        gpiblpt('selectedDeviceClear',...
          [this.PrimaryAddress],...
          max([this.timeout]));
      catch e
        this.error(e);
      end
    end
    
    function trigger(this)
      try
        gpiblpt('groupExecuteTrigger',...
          [this.PrimaryAddress],...
          max([this.timeout]));
      catch e
        this.error(e);
      end
    end
    
    function clralldevices(this)
      try
        gpiblpt('deviceClear',this.timeout);
      catch e
        this.error(e);
      end
    end
    
    function SRQ = get.ServiceRequest(this)
      try
        SRQ = gpiblpt('readSRQ',this.timeout);
      catch e
        this.error(e);
      end
    end
    
    function [o,statusByte] = spoll(this)
      try
        gpiblpt('serialPollEnable',this.timeout);
      catch e
        this.error(e);
      end
      o = uint8([]);
      for i = uint8(1:31)
        try %#ok<TRYNC>
          statusByte = gpiblpt('serialPoll',i,uint16(15));
          if ~isempty(statusByte)
            o = GPIBlpt(i);
            break;
          end
        end
      end
      try
        gpiblpt('serialPollDisable',this.timeout);
      catch e
        this.error(e);
      end
    end
    
  end
  
  methods (Static, Access = private)
    
    function error(e)
      if regexp(e.identifier,'^gpiblpt:.*:E1460$','once')
        s = dbstack(1);
        e = MException(sprintf('GPIBlpt:%s:Timeout',s(1).name(9:end)),...
          'Bus timeout occurred.');
        throwAsCaller(e);
      else
        rethrow(e);
      end
    end
    
  end
  
end
