function varargout = gui_HelicopterTrim(varargin)
% GUI_HELICOPTERTRIM M-file for gui_HelicopterTrim.fig
%      GUI_HELICOPTERTRIM, by itself, creates a new GUI_HELICOPTERTRIM or raises the existing
%      singleton*.
%
%      H = GUI_HELICOPTERTRIM returns the handle to a new GUI_HELICOPTERTRIM or the handle to
%      the existing singleton*.
%
%      GUI_HELICOPTERTRIM('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in GUI_HELICOPTERTRIM.M with the given input arguments.
%
%      GUI_HELICOPTERTRIM('Property','Value',...) creates a new GUI_HELICOPTERTRIM or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before gui_HelicopterTrim_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to gui_HelicopterTrim_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help gui_HelicopterTrim

% Last Modified by GUIDE v2.5 27-Sep-2011 09:53:58

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @gui_HelicopterTrim_OpeningFcn, ...
                   'gui_OutputFcn',  @gui_HelicopterTrim_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before gui_HelicopterTrim is made visible.
function gui_HelicopterTrim_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to gui_HelicopterTrim (see VARARGIN)

% Choose default command line output for gui_HelicopterTrim
handles.output = hObject;

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes gui_HelicopterTrim wait for user response (see UIRESUME)
% uiwait(handles.figure1);
a = evalin('base', 'exist(''helicopter_trim.mat'', ''file'');'); 
if a
    evalin('base', 'load helicopter_trim.mat');
    
    a = evalin('base', 'in_N;');                    set(handles.data_points,        'String', mat2str(a+1));
    a = evalin('base', 'in_omega_MR;');             set(handles.rpm_mr,             'String', mat2str(a));
    a = evalin('base', 'in_omega_TR;');             set(handles.rpm_tr,             'String', mat2str(a));
    a = evalin('base', 'in_pressure_altitude;');    set(handles.pressure_altitude,  'String', mat2str(a));
    a = evalin('base', 'in_outside_air_temp;');     set(handles.outside_air_temp,   'String', mat2str(a));
    a = evalin('base', 'in_CMD_ini;');
    set(handles.collective,   'String', mat2str(a(1)));
    set(handles.stick_a1,     'String', mat2str(a(2)));
    set(handles.stick_b1,     'String', mat2str(a(3)));
    set(handles.pedals, 'String', mat2str(a(4)));
    
    a = evalin('base', 'in_xc_EOM.signals(1).values(1,:);');
    b = evalin('base', 'in_d.signals(1).values(1,:);');
    a = reshape([a'; b'], 3, 9)';
    data = get(handles.table_ini_states, 'Data');
    for i=1:9
        for j=1:3
            data{i, j} = a(i, j);
        end
    end
    set(handles.table_ini_states, 'Data', data);
    
    a = evalin('base', 'in_xc_EOM.signals(1).values(2,:);');
    b = evalin('base', 'in_d.signals(1).values(2,:);');
    a = reshape([a'; b'], 3, 9)';
    data = get(handles.table_fin_states, 'Data');
    for i=1:9
        for j=1:3
            data{i, j} = a(i, j);
        end
    end
    set(handles.table_fin_states, 'Data', data);
end



% --- Outputs from this function are returned to the command line.
function varargout = gui_HelicopterTrim_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;



function pressure_altitude_Callback(hObject, eventdata, handles)
% hObject    handle to pressure_altitude (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of pressure_altitude as text
%        str2double(get(hObject,'String')) returns contents of pressure_altitude as a double


% --- Executes during object creation, after setting all properties.
function pressure_altitude_CreateFcn(hObject, eventdata, handles)
% hObject    handle to pressure_altitude (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function outside_air_temp_Callback(hObject, eventdata, handles)
% hObject    handle to outside_air_temp (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of outside_air_temp as text
%        str2double(get(hObject,'String')) returns contents of outside_air_temp as a double


% --- Executes during object creation, after setting all properties.
function outside_air_temp_CreateFcn(hObject, eventdata, handles)
% hObject    handle to outside_air_temp (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function collective_Callback(hObject, eventdata, handles)
% hObject    handle to collective (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of collective as text
%        str2double(get(hObject,'String')) returns contents of collective as a double


% --- Executes during object creation, after setting all properties.
function collective_CreateFcn(hObject, eventdata, handles)
% hObject    handle to collective (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function stick_a1_Callback(hObject, eventdata, handles)
% hObject    handle to stick_a1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of stick_a1 as text
%        str2double(get(hObject,'String')) returns contents of stick_a1 as a double


% --- Executes during object creation, after setting all properties.
function stick_a1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to stick_a1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function stick_b1_Callback(hObject, eventdata, handles)
% hObject    handle to stick_b1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of stick_b1 as text
%        str2double(get(hObject,'String')) returns contents of stick_b1 as a double


% --- Executes during object creation, after setting all properties.
function stick_b1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to stick_b1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function pedals_Callback(hObject, eventdata, handles)
% hObject    handle to pedals (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of pedals as text
%        str2double(get(hObject,'String')) returns contents of pedals as a double


% --- Executes during object creation, after setting all properties.
function pedals_CreateFcn(hObject, eventdata, handles)
% hObject    handle to pedals (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function rpm_mr_Callback(hObject, eventdata, handles)
% hObject    handle to rpm_mr (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of rpm_mr as text
%        str2double(get(hObject,'String')) returns contents of rpm_mr as a double


% --- Executes during object creation, after setting all properties.
function rpm_mr_CreateFcn(hObject, eventdata, handles)
% hObject    handle to rpm_mr (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function rpm_tr_Callback(hObject, eventdata, handles)
% hObject    handle to rpm_tr (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of rpm_tr as text
%        str2double(get(hObject,'String')) returns contents of rpm_tr as a double


% --- Executes during object creation, after setting all properties.
function rpm_tr_CreateFcn(hObject, eventdata, handles)
% hObject    handle to rpm_tr (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in button_configure_parameters.
function button_configure_parameters_Callback(hObject, eventdata, handles)
% hObject    handle to button_configure_parameters (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)


% --- Executes on button press in button_ok.
function button_ok_Callback(hObject, eventdata, handles)
% hObject    handle to button_ok (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

Np = str2num(get(handles.data_points, 'String'));
Np = Np - 1;
evalin('base', ['in_N=       '          get(handles.data_points,        'String') '-1;']);
evalin('base', ['in_omega_MR='          get(handles.rpm_mr,             'String') ';']);
evalin('base', ['in_omega_TR='          get(handles.rpm_tr,             'String') ';']);
evalin('base', ['in_pressure_altitude=' get(handles.pressure_altitude,  'String') ';']);
evalin('base', ['in_outside_air_temp='  get(handles.outside_air_temp,   'String') ';']);
evalin('base', ['in_CMD_ini=['          get(handles.collective,         'String') ';' ...
                                        get(handles.stick_a1,           'String') ';' ...
                                        get(handles.stick_b1,           'String') ';' ...
                                        get(handles.pedals,             'String') '];']);

data = get(handles.table_ini_states, 'Data');
data = data';
xc_EOM1 = [data{1:3, 1:4}];
d1 =      [data{1:3, 5:9}];
data = get(handles.table_fin_states, 'Data');
data = data';
xc_EOM2 = [data{1:3, 1:4}];
d2 =      [data{1:3, 5:9}];

evalin('base', ['in_xc_EOM.time='    mat2str([0 Np]') ';']);
evalin('base', ['in_xc_EOM.signals(1).values='    mat2str([xc_EOM1; xc_EOM2]) ';']);
evalin('base', 'in_xc_EOM.dimensions=2;');

evalin('base', ['in_d.time='    mat2str([0 Np]') ';']);
evalin('base', ['in_d.signals(1).values='    mat2str([d1; d2]) ';']);
evalin('base', 'in_d.dimensions=2;');

evalin('base', 'save helicopter_trim.mat in_N in_omega_MR in_omega_TR in_pressure_altitude in_outside_air_temp in_CMD_ini in_xc_EOM in_d');
delete(handles.figure1);


% --- Executes on button press in button_cancel.
function button_cancel_Callback(hObject, eventdata, handles)
% hObject    handle to button_cancel (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
delete(handles.figure1);



function data_points_Callback(hObject, eventdata, handles)
% hObject    handle to data_points (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of data_points as text
%        str2double(get(hObject,'String')) returns contents of data_points as a double


% --- Executes during object creation, after setting all properties.
function data_points_CreateFcn(hObject, eventdata, handles)
% hObject    handle to data_points (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end
