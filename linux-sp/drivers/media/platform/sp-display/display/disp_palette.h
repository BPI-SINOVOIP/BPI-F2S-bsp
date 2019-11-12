#ifndef __DISP_PALETTE_H__
#define __DISP_PALETTE_H__

/*
    OSD path , fmt = 8bpp , palette setting for grey scale (alpha default set as 0xff)

    idx000 : dark black
    idx008 : ...
    idx016 : ...
    idx024 : ...
    idx032 : ...
    idx040 : ...
    idx048 : ...
    idx056 : black
    idx064 : ...
    idx072 : ...
    idx080 : ...
    idx088 : ...
    idx096 : ...
    idx104 : ...
    idx112 : ...
    idx120 : grey
    idx128 : ...
    idx136 : ...
    idx144 : ...
    idx152 : ...
    idx160 : ...
    idx168 : ...
    idx176 : ...
    idx184 : white
    idx192 : ...
    idx200 : ...
    idx208 : ...
    idx216 : ...
    idx224 : ...
    idx232 : ...
    idx240 : ...
    idx248 : bright white

    data format : ARGB means Alpha Red Green Blue 
                    0xff474747 (Alpha 0xff , Red 0x47 , Green 0x47 , Blue 0x47)

 */
u32 disp_osd_8bpp_pal_grey[256] = { //8bpp ARGB
    0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 ,
    0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 , 0xff000000 ,
    0xff000000 , 0xff010101 , 0xff020202 , 0xff030303 , 0xff040404 , 0xff050505 , 0xff060606 , 0xff080808 ,
    0xff090909 , 0xff0a0a0a , 0xff0b0b0b , 0xff0c0c0c , 0xff0d0d0d , 0xff0f0f0f , 0xff101010 , 0xff111111 ,
    0xff121212 , 0xff131313 , 0xff141414 , 0xff161616 , 0xff171717 , 0xff181818 , 0xff191919 , 0xff1a1a1a ,
    0xff1b1b1b , 0xff1d1d1d , 0xff1e1e1e , 0xff1f1f1f , 0xff202020 , 0xff212121 , 0xff222222 , 0xff242424 ,
    0xff252525 , 0xff262626 , 0xff272727 , 0xff282828 , 0xff292929 , 0xff2b2b2b , 0xff2c2c2c , 0xff2d2d2d ,
    0xff2e2e2e , 0xff2f2f2f , 0xff303030 , 0xff323232 , 0xff333333 , 0xff343434 , 0xff353535 , 0xff363636 ,
    0xff373737 , 0xff393939 , 0xff3a3a3a , 0xff3b3b3b , 0xff3c3c3c , 0xff3d3d3d , 0xff3e3e3e , 0xff404040 ,
    0xff414141 , 0xff424242 , 0xff434343 , 0xff444444 , 0xff454545 , 0xff474747 , 0xff484848 , 0xff494949 ,
    0xff4a4a4a , 0xff4b4b4b , 0xff4c4c4c , 0xff4d4d4d , 0xff4f4f4f , 0xff505050 , 0xff515151 , 0xff525252 ,
    0xff535353 , 0xff545454 , 0xff565656 , 0xff575757 , 0xff585858 , 0xff595959 , 0xff5a5a5a , 0xff5b5b5b ,
    0xff5d5d5d , 0xff5e5e5e , 0xff5f5f5f , 0xff606060 , 0xff616161 , 0xff626262 , 0xff646464 , 0xff656565 ,
    0xff666666 , 0xff676767 , 0xff686868 , 0xff696969 , 0xff6b6b6b , 0xff6c6c6c , 0xff6d6d6d , 0xff6e6e6e ,
    0xff6f6f6f , 0xff707070 , 0xff727272 , 0xff737373 , 0xff747474 , 0xff757575 , 0xff767676 , 0xff777777 ,
    0xff797979 , 0xff7a7a7a , 0xff7b7b7b , 0xff7c7c7c , 0xff7d7d7d , 0xff7e7e7e , 0xff808080 , 0xff818181 ,
    0xff828282 , 0xff838383 , 0xff848484 , 0xff858585 , 0xff878787 , 0xff888888 , 0xff898989 , 0xff8a8a8a ,
    0xff8b8b8b , 0xff8c8c8c , 0xff8e8e8e , 0xff8f8f8f , 0xff909090 , 0xff919191 , 0xff929292 , 0xff939393 ,
    0xff949494 , 0xff969696 , 0xff979797 , 0xff989898 , 0xff999999 , 0xff9a9a9a , 0xff9b9b9b , 0xff9d9d9d ,
    0xff9e9e9e , 0xff9f9f9f , 0xffa0a0a0 , 0xffa1a1a1 , 0xffa2a2a2 , 0xffa4a4a4 , 0xffa5a5a5 , 0xffa6a6a6 ,
    0xffa7a7a7 , 0xffa8a8a8 , 0xffa9a9a9 , 0xffababab , 0xffacacac , 0xffadadad , 0xffaeaeae , 0xffafafaf ,
    0xffb0b0b0 , 0xffb2b2b2 , 0xffb3b3b3 , 0xffb4b4b4 , 0xffb5b5b5 , 0xffb6b6b6 , 0xffb7b7b7 , 0xffb9b9b9 ,
    0xffbababa , 0xffbbbbbb , 0xffbcbcbc , 0xffbdbdbd , 0xffbebebe , 0xffc0c0c0 , 0xffc1c1c1 , 0xffc2c2c2 ,
    0xffc3c3c3 , 0xffc4c4c4 , 0xffc5c5c5 , 0xffc7c7c7 , 0xffc8c8c8 , 0xffc9c9c9 , 0xffcacaca , 0xffcbcbcb ,
    0xffcccccc , 0xffcecece , 0xffcfcfcf , 0xffd0d0d0 , 0xffd1d1d1 , 0xffd2d2d2 , 0xffd3d3d3 , 0xffd5d5d5 ,
    0xffd6d6d6 , 0xffd7d7d7 , 0xffd8d8d8 , 0xffd9d9d9 , 0xffdadada , 0xffdbdbdb , 0xffdddddd , 0xffdedede ,
    0xffdfdfdf , 0xffe0e0e0 , 0xffe1e1e1 , 0xffe2e2e2 , 0xffe4e4e4 , 0xffe5e5e5 , 0xffe6e6e6 , 0xffe7e7e7 ,
    0xffe8e8e8 , 0xffe9e9e9 , 0xffebebeb , 0xffececec , 0xffededed , 0xffefefef , 0xfff0f0f0 , 0xfff2f2f2 ,
    0xfff3f3f3 , 0xfff4f4f4 , 0xfff5f5f5 , 0xfff6f6f6 , 0xfff7f7f7 , 0xfff9f9f9 , 0xfffafafa , 0xfffbfbfb ,
    0xfffcfcfc , 0xfffdfdfd , 0xfffefefe , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff ,
    0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff ,
    0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff
};

/*

    OSD path , fmt = 8bpp , palette setting for 256 color (alpha default set as 0xff)

    idx000 : Black (SYSTEM) Maroon (SYSTEM) Green (SYSTEM) Olive (SYSTEM) Navy (SYSTEM) Purple (SYSTEM) Teal (SYSTEM) Silver (SYSTEM)
    idx008 : Grey (SYSTEM) Red (SYSTEM) Lime (SYSTEM) Yellow (SYSTEM) Blue (SYSTEM) Fuchsia (SYSTEM) Aqua (SYSTEM) White (SYSTEM)
    idx016 : Grey0 NavyBlue DarkBlue Blue3 Blue3 Blue1 DarkGreen DeepSkyBlue4
    idx024 : DeepSkyBlue4 DeepSkyBlue4 DodgerBlue3 DodgerBlue2 Green4 SpringGreen4 Turquoise4 DeepSkyBlue3
    idx032 : DeepSkyBlue3 DodgerBlue1 Green3 SpringGreen3 DarkCyan LightSeaGreen DeepSkyBlue2 DeepSkyBlue1
    idx040 : Green3 SpringGreen3 SpringGreen2 Cyan3 DarkTurquoise Turquoise2 Green1 SpringGreen2
    idx048 : SpringGreen1 MediumSpringGreen Cyan2 Cyan1 DarkRed DeepPink4 Purple4 Purple4
    idx056 : Purple3 BlueViolet Orange4 Grey37 MediumPurple4 SlateBlue3 SlateBlue3 RoyalBlue1
    idx064 : Chartreuse4 DarkSeaGreen4 PaleTurquoise4 SteelBlue SteelBlue3 CornflowerBlue Chartreuse3 DarkSeaGreen4
    idx072 : CadetBlue CadetBlue SkyBlue3 SteelBlue1 Chartreuse3 PaleGreen3 SeaGreen3 Aquamarine3
    idx080 : MediumTurquoise SteelBlue1 Chartreuse2 SeaGreen2 SeaGreen1 SeaGreen1 Aquamarine1 DarkSlateGray2
    idx088 : DarkRed DeepPink4 DarkMagenta DarkMagenta DarkViolet Purple Orange4 LightPink4
    idx096 : Plum4 MediumPurple3 MediumPurple3 SlateBlue1 Yellow4 Wheat4 Grey53 LightSlateGrey
    idx104 : MediumPurple LightSlateBlue Yellow4 DarkOliveGreen3 DarkSeaGreen LightSkyBlue3 LightSkyBlue3 SkyBlue2
    idx112 : Chartreuse2 DarkOliveGreen3 PaleGreen3 DarkSeaGreen3 DarkSlateGray3 SkyBlue1 Chartreuse1 LightGreen
    idx120 : LightGreen PaleGreen1 Aquamarine1 DarkSlateGray1 Red3 DeepPink4 MediumVioletRed Magenta3
    idx128 : DarkViolet Purple DarkOrange3 IndianRed HotPink3 MediumOrchid3 MediumOrchid MediumPurple2
    idx136 : DarkGoldenrod LightSalmon3 RosyBrown Grey63 MediumPurple2 MediumPurple1 Gold3 DarkKhaki
    idx144 : NavajoWhite3 Grey69 LightSteelBlue3 LightSteelBlue Yellow3 DarkOliveGreen3 DarkSeaGreen3 DarkSeaGreen2
    idx152 : LightCyan3 LightSkyBlue1 GreenYellow DarkOliveGreen2 PaleGreen1 DarkSeaGreen2 DarkSeaGreen1 PaleTurquoise1
    idx160 : Red3 DeepPink3 DeepPink3 Magenta3 Magenta3 Magenta2 DarkOrange3 IndianRed
    idx168 : HotPink3 HotPink2 Orchid MediumOrchid1 Orange3 LightSalmon3 LightPink3 Pink3
    idx176 : Plum3 Violet Gold3 LightGoldenrod3 Tan MistyRose3 Thistle3 Plum2
    idx184 : Yellow3 Khaki3 LightGoldenrod2 LightYellow3 Grey84 LightSteelBlue1 Yellow2 DarkOliveGreen1
    idx192 : DarkOliveGreen1 DarkSeaGreen1 Honeydew2 LightCyan1 Red1 DeepPink2 DeepPink1 DeepPink1
    idx200 : Magenta2 Magenta1 OrangeRed1 IndianRed1 IndianRed1 HotPink HotPink MediumOrchid1
    idx208 : DarkOrange Salmon1 LightCoral PaleVioletRed1 Orchid2 Orchid1 Orange1 SandyBrown
    idx216 : LightSalmon1 LightPink1 Pink1 Plum1 Gold1 LightGoldenrod2 LightGoldenrod2 NavajoWhite1
    idx224 : MistyRose1 Thistle1 Yellow1 LightGoldenrod1 Khaki1 Wheat1 Cornsilk1 Grey100
    idx232 : Grey3 Grey7 Grey11 Grey15 Grey19 Grey23 Grey27 Grey30
    idx240 : Grey35 Grey39 Grey42 Grey46 Grey50 Grey54 Grey58 Grey62
    idx248 : Grey66 Grey70 Grey74 Grey78 Grey82 Grey85 Grey89 Grey93

    data format : ARGB means Alpha Red Green Blue 
                    0xff008000 (Alpha 0xff , Red 0x00 , Green 0x80 , Blue 0x00)

 */
u32 disp_osd_8bpp_pal_color[256] = { //8bpp ARGB
    0xff000000 , 0xff800000 , 0xff008000 , 0xff808000 , 0xff000080 , 0xff800080 , 0xff008080 , 0xffc0c0c0 ,
    0xff808080 , 0xffff0000 , 0xff00ff00 , 0xffffff00 , 0xff0000ff , 0xffff00ff , 0xff00ffff , 0xffffffff , 
    0xff000000 , 0xff00005f , 0xff000087 , 0xff0000af , 0xff0000d7 , 0xff0000ff , 0xff005f00 , 0xff005f5f , 
    0xff005f87 , 0xff005faf , 0xff005fd7 , 0xff005fff , 0xff008700 , 0xff00875f , 0xff008787 , 0xff0087af , 
    0xff0087d7 , 0xff0087ff , 0xff00af00 , 0xff00af5f , 0xff00af87 , 0xff00afaf , 0xff00afd7 , 0xff00afff , 
    0xff00d700 , 0xff00d75f , 0xff00d787 , 0xff00d7af , 0xff00d7d7 , 0xff00d7ff , 0xff00ff00 , 0xff00ff5f , 
    0xff00ff87 , 0xff00ffaf , 0xff00ffd7 , 0xff00ffff , 0xff5f0000 , 0xff5f005f , 0xff5f0087 , 0xff5f00af , 
    0xff5f00d7 , 0xff5f00ff , 0xff5f5f00 , 0xff5f5f5f , 0xff5f5f87 , 0xff5f5faf , 0xff5f5fd7 , 0xff5f5fff , 
    0xff5f8700 , 0xff5f875f , 0xff5f8787 , 0xff5f87af , 0xff5f87d7 , 0xff5f87ff , 0xff5faf00 , 0xff5faf5f , 
    0xff5faf87 , 0xff5fafaf , 0xff5fafd7 , 0xff5fafff , 0xff5fd700 , 0xff5fd75f , 0xff5fd787 , 0xff5fd7af , 
    0xff5fd7d7 , 0xff5fd7ff , 0xff5fff00 , 0xff5fff5f , 0xff5fff87 , 0xff5fffaf , 0xff5fffd7 , 0xff5fffff , 
    0xff870000 , 0xff87005f , 0xff870087 , 0xff8700af , 0xff8700d7 , 0xff8700ff , 0xff875f00 , 0xff875f5f , 
    0xff875f87 , 0xff875faf , 0xff875fd7 , 0xff875fff , 0xff878700 , 0xff87875f , 0xff878787 , 0xff8787af , 
    0xff8787d7 , 0xff8787ff , 0xff87af00 , 0xff87af5f , 0xff87af87 , 0xff87afaf , 0xff87afd7 , 0xff87afff , 
    0xff87d700 , 0xff87d75f , 0xff87d787 , 0xff87d7af , 0xff87d7d7 , 0xff87d7ff , 0xff87ff00 , 0xff87ff5f , 
    0xff87ff87 , 0xff87ffaf , 0xff87ffd7 , 0xff87ffff , 0xffaf0000 , 0xffaf005f , 0xffaf0087 , 0xffaf00af , 
    0xffaf00d7 , 0xffaf00ff , 0xffaf5f00 , 0xffaf5f5f , 0xffaf5f87 , 0xffaf5faf , 0xffaf5fd7 , 0xffaf5fff , 
    0xffaf8700 , 0xffaf875f , 0xffaf8787 , 0xffaf87af , 0xffaf87d7 , 0xffaf87ff , 0xffafaf00 , 0xffafaf5f , 
    0xffafaf87 , 0xffafafaf , 0xffafafd7 , 0xffafafff , 0xffafd700 , 0xffafd75f , 0xffafd787 , 0xffafd7af , 
    0xffafd7d7 , 0xffafd7ff , 0xffafff00 , 0xffafff5f , 0xffafff87 , 0xffafffaf , 0xffafffd7 , 0xffafffff , 
    0xffd70000 , 0xffd7005f , 0xffd70087 , 0xffd700af , 0xffd700d7 , 0xffd700ff , 0xffd75f00 , 0xffd75f5f , 
    0xffd75f87 , 0xffd75faf , 0xffd75fd7 , 0xffd75fff , 0xffd78700 , 0xffd7875f , 0xffd78787 , 0xffd787af , 
    0xffd787d7 , 0xffd787ff , 0xffd7af00 , 0xffd7af5f , 0xffd7af87 , 0xffd7afaf , 0xffd7afd7 , 0xffd7afff , 
    0xffd7d700 , 0xffd7d75f , 0xffd7d787 , 0xffd7d7af , 0xffd7d7d7 , 0xffd7d7ff , 0xffd7ff00 , 0xffd7ff5f , 
    0xffd7ff87 , 0xffd7ffaf , 0xffd7ffd7 , 0xffd7ffff , 0xffff0000 , 0xffff005f , 0xffff0087 , 0xffff00af , 
    0xffff00d7 , 0xffff00ff , 0xffff5f00 , 0xffff5f5f , 0xffff5f87 , 0xffff5faf , 0xffff5fd7 , 0xffff5fff , 
    0xffff8700 , 0xffff875f , 0xffff8787 , 0xffff87af , 0xffff87d7 , 0xffff87ff , 0xffffaf00 , 0xffffaf5f , 
    0xffffaf87 , 0xffffafaf , 0xffffafd7 , 0xffffafff , 0xffffd700 , 0xffffd75f , 0xffffd787 , 0xffffd7af , 
    0xffffd7d7 , 0xffffd7ff , 0xffffff00 , 0xffffff5f , 0xffffff87 , 0xffffffaf , 0xffffffd7 , 0xffffffff , 
    0xff080808 , 0xff121212 , 0xff1c1c1c , 0xff262626 , 0xff303030 , 0xff3a3a3a , 0xff444444 , 0xff4e4e4e , 
    0xff585858 , 0xff626262 , 0xff6c6c6c , 0xff767676 , 0xff808080 , 0xff8a8a8a , 0xff949494 , 0xff9e9e9e , 
    0xffa8a8a8 , 0xffb2b2b2 , 0xffbcbcbc , 0xffc6c6c6 , 0xffd0d0d0 , 0xffdadada , 0xffe4e4e4 , 0xffeeeeee
};
#endif	//__DISP_DEBUG_H__

