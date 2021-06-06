clc
clear
close all

%Çift Pulse Farklı bir kod gerektiriyor?
%display_image('LOC_AX_SE_I.txt','LOC_AX_SE_Q.txt')%,30,)
%display_image('LOC_AX_SE2_I.txt','LOC_AX_SE2_Q.txt',30,15,32)
%display_image('LOC_AX_SE3_I.txt','LOC_AX_SE3_Q.txt',2599572,7.88,6,16)

%display_image('Scout_I.txt','Scout_Q.txt',4768199,15.0985,6,32);% ABS/REAL PLOT BOZUK
%display_image('Scout2_I.txt','Scout2_Q.txt',3827100,15.0985,6,16)
%display_image('ugurMod_I.txt','ugurMod_Q.txt',3926392,30,15,32) % BOZUK
%display_image('ugurMod2_I.txt','ugurMod2_Q.txt',3926391,30.3313,15,32,236) %DATA EKSIK, BOZUK
%display_image('ugurMod3_I.txt','ugurMod3_Q.txt',3500804,20.333,10,32,256)
%display_image('ugurMod5_I.txt','ugurMod5_Q.txt',43801,20.33,10,32,256)

display_image('ugurMod6_saggital_I.txt','ugurMod6_saggital_Q.txt',112210,20.3313,10,32,256)

function [] = display_image(f_I,f_Q,t_i,TR1,TE1,DW1,Ny1)
    fname_I = f_I;
    f = fopen(fname_I);
    d = fread(f,'uchar');
    nI = str2num(convertCharsToStrings(char(d)));
    
    fname_Q = f_Q;
    f = fopen(fname_Q);
    d = fread(f,'uchar');
    nQ = str2num(convertCharsToStrings(char(d)));
   
    
    data = nI(2:end) + 1i*nQ(2:end);
    %demod_nI = shift_demod(nI(2:end),1000,0.5*1e6);
    %demod_nQ = shift_demod(nQ(2:end),1000,0.5*1e6);
    %data = demod_nI + 1i*demod_nQ;
    figure;
    %plot(real(data(3926391+2046000:6950000)));
    plot(abs(data(112210:120000)));
    
    
    t0 = t_i;
    fre = 500;
    TR = TR1*fre;
    TE = TE1*fre;
    DW = DW1;
    Nx = 256;
    Ny = Ny1;
    readout = DW*Nx*fre/1000;
    
    mydata = zeros(readout,Ny);
    for n=1:Ny
        datastart = t0+(n-1)*TR+TE-readout/2;
        mydata(:,n) = data(datastart:(datastart+readout-1));
    end
    
    figure;
    imagesc(abs(mydata))
    size(mydata)
    figure;
    myimage = fft2(mydata)/1e3;
    max(max(abs(myimage)))
    for n=1:Ny
        fftshift_myimage(:,n) = fftshift(myimage(:,n));
    end
    
    cropstart = readout/2-Nx/2;
    image(abs(fftshift_myimage(cropstart-80:(cropstart-80+Ny-1),:))')
    colormap(gray);
    figure;
    image(abs(fftshift_myimage(:,:)))
    colormap(gray);

end

function [shifted_x] = shift_demod(x,shift_f,fs)
    size_x = size(x);
    n = 1:1:size_x(2);
    exp_func = exp(2*pi*1i*shift_f*(1/fs)*n);
    shifted_x = x.*exp_func;
    
end


