/****************************************************************

   Example C++ script to convert a raw data .tpx3 to text file

        copyright JL@ Amsterdam Scientific Instruments B.V.
                      www.amscins.com
                        05-12-2019
****************************************************************/


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace std;


#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

// #include <pybind11/stl.h>
// #include <pybind11/stl_bind.h>

namespace py = pybind11;


#include "dtypes.h"

py::array_t<TPXHit> parse_binary_file(std::string filename)
{  


    // ofstream xy_file("converted.txt");   //Converted and saved txt file
    streampos begin, end;
    ifstream myfile(filename.c_str(), ios::binary);
    unsigned short xpix, ypix, TOT, TOA, spidrTime;
    char chipnr, FTOA;
    int frameNr;
    int CTOA;
    int mode;
    unsigned long Timer_LSB32 = 0;
    unsigned long Timer_MSB16 = 0;
    unsigned long numofTDC=0;

    std::vector<TPXHit> output;
    output.reserve(1000);

    if (!myfile) {
        cout << "This file is not found!" << endl;
    }
    else {
        myfile.seekg(0, myfile.end);
        unsigned long long fileLength = myfile.tellg();
        myfile.seekg(0, myfile.beg);
        unsigned long long temp64;
        // cout << "filesize: " << fileLength/(1024*1024) <<"MB" << endl;
        unsigned long long NumofPacket = fileLength / 8;
        unsigned long long* datapacket = new unsigned long long [NumofPacket];
        myfile.read((char*) datapacket, fileLength);
        myfile.close();
        char* HeaderBuffer = new char[8];
        unsigned long long temp;
        
        for (unsigned long long i = 0; i < NumofPacket; i++) {
            // cout << "i: " << i << endl;
            memcpy(HeaderBuffer, &datapacket[i], 8);            
            if (HeaderBuffer[0] == 'T' && HeaderBuffer[1] == 'P' && HeaderBuffer[2] == 'X') {
                int size = ((0xff & HeaderBuffer[7]) << 8) | (0xff & HeaderBuffer[6]);
                chipnr = HeaderBuffer[4];
                mode = HeaderBuffer[5];
                for (int j = 0; j < size / 8; j++) {
                    temp = datapacket[i + j + 1];
                    int hdr = (int)(temp >> 56);
                    int packet = temp >> 60;
                    double coarsetime;
                    unsigned long tmpfine;
                    unsigned long trigtime_fine;
                    double time_unit, global_timestamp;
                    int trigger_counter;
                    unsigned long long int timemaster;
                    int heartbeatL, heartbeatM;
                    double TDC_timestamp;
                    double spidrTimens;
                    int x, y;
                    double TOTns;
                    double TOAns;
                    long dcol;
                    long spix;
                    long pix;
                    
                    switch (packet)
                    {
                    case 0x6:      //TDC timestamp packet header
                       
                        if ((temp >> 56) == 0x6f) cout << "tdc1 rising edge is working" << endl; 
                        if ((temp >> 56) == 0x6a) cout << "tdc1 falling edge is working" << endl;
                        if ((temp >> 56) == 0x6e) cout << "tdc2 rising edge is working" << endl;
                        if ((temp >> 56) == 0x6b) cout << "tdc2 falling edge is working" << endl;                      
                        coarsetime = (temp >> 12) & 0xFFFFFFFF;                        
                        tmpfine = (temp >> 5) & 0xF; 
                        tmpfine = ((tmpfine - 1) << 9) / 12;
                        trigtime_fine = (temp & 0x0000000000000E00) | (tmpfine & 0x00000000000001FF);
                        time_unit = 25. / 4096;
                        trigger_counter = temp >> 44 & 0xFFF; 
                        TDC_timestamp = coarsetime * 25E-9 + trigtime_fine * time_unit*1E-9;
                        //uncomment below to save TDC timestamps into the txt file                        
                        // xy_file << setprecision(15) << TDC_timestamp << endl;  
                        // cout<< "TDC timestamp: " << setprecision(15) << TDC_timestamp << endl;
                        numofTDC=numofTDC+1;                                                
                        break;

                    case 0xb:       //Chip data: ToA and ToT timestamp packet, x, y

                        spidrTime = (unsigned short)(temp & 0xffff);
                        dcol = (temp & 0x0FE0000000000000L) >> 52;                                                                  
                        spix = (temp & 0x001F800000000000L) >> 45;                                                                    
                        pix = (temp & 0x0000700000000000L) >> 44;
                        x = (int)(dcol + pix / 4);
                        y = (int)(spix + (pix & 0x3));
                        TOA = (unsigned short)((temp >> (16 + 14)) & 0x3fff);   
                        TOT = (unsigned short)((temp >> (16 + 4)) & 0x3ff); 
                        FTOA = (unsigned char)((temp >> 16) & 0xf);
                        CTOA = (TOA << 4) | (~FTOA & 0xf);
                        spidrTimens = spidrTime * 25.0 * 16384.0;
                        TOAns = TOA * 25.0;
                        TOTns = TOT * 25.0; 
                        global_timestamp = spidrTimens + CTOA * (25.0 / 16);
                        
                        /************************************************************ 
                        Condition is different for single Timepix3 chip or quad chips:                      
                        Single chip, using "int (Chipnr) +3"                        
                        Quad chips, using "int (Chipnr)"                        
                        ************************************************************/                       
                        switch (int (chipnr)) // for quad chips;
                        {

                        case 0:
                            x += 260;
                            y = y;
                            break;

                        case 1:
                            x = 255 - x + 260;
                            y = 255 - y + 260;
                            break;

                        case 2:
                            x = 255 - x;
                            y = 255 - y + 260;
                            break;

                        case 3:
                            break;

                        default:
                            break;
                            
                        }

                        //uncomment below to save the chip data into the text file;
                        // xy_file << setprecision(15) << global_timestamp / 1E9 << "  " << TOTns << "  " << x << "  " << y << endl;   //toa, tot, x and y data can be saved into txt data
                        // cout<< "Chip-ToA: " << setprecision(15) << global_timestamp / 1E9 << "  ToT: " << TOTns << " x: " << x << " y: " << y << endl;
                        output.push_back(TPXHit(global_timestamp, TOTns, x, y));
                        break;

                    case 0x4:       //the global timestamps.

                        if (((temp >> 56) & 0xF) == 0x4) {
                            Timer_LSB32 = (temp >> 16) & 0xFFFFFFFF;
                        }
                        else if (((temp >> 56) & 0xF) == 0x5)
                        {
                            Timer_MSB16 = (temp >> 16) & 0xFFFF;
                            unsigned long long int timemaster;
                            timemaster = Timer_MSB16;
                            timemaster = (timemaster << 32) & 0xFFFF00000000;
                            timemaster = timemaster | Timer_LSB32;
                            int diff = (spidrTime >> 14) - ((Timer_LSB32 >> 28) & 0x3);

                            if ((spidrTime >> 14) == ((Timer_LSB32 >> 28) & 0x3))
                            {                       
                            }
                            else {                               
                                Timer_MSB16 = Timer_MSB16 - diff;
                            }  
                            //uncomment below to save the global timestamps into the text file;
                            // xy_file << " Global time: " << setprecision(15) << timemaster * 25e-9 << endl;  //global timestamps can be saved into text file
                        }

                        break;

                    default:
                        break;
                    }

                }
                i += (size / 8);
                
                // printf("i : %lld\r", i);
            }
        }

        delete  [] HeaderBuffer;
        delete [] datapacket;
    }
    // cout<<"the number of TDCs: "<<numofTDC<<endl;
    // xy_file.close();
    // cout << "finished! " << endl;

    // // Convert the output to a numpy buffer:
    // auto result = py::array_t<TPXHit>(output.size());

    // TPXHit *ptr_result = static_cast<TPXHit *>(result.ptr);
    // // double *ptr2 = static_cast<double *>(buf2.ptr);
    // // double *ptr3 = static_cast<double *>(buf3.ptr);

    // // for (size_t idx = 0; idx < buf1.shape[0]; idx++)
    // //     ptr3[idx] = ptr1[idx] + ptr2[idx];

    // for (int i = 0; i < output.size(); ++i)
    // {
    //     ptr_result[i] = output.at(i);
    // }

    py::array result(output.size(), output.data());

    return result;
}


PYBIND11_MODULE(tpx, m) {
    // ...

    PYBIND11_NUMPY_DTYPE(TPXHit, TOA, TOT, x, y);

    m.def("parse_binary_file", &parse_binary_file);
    // PYBIND11_NUMPY_DTYPE(B, z, a);
    /* now both A and B can be used as template arguments to py::array_t */

    // py::class_<TPXFrame>(m, "TPXFrame", py::buffer_protocol())
    // .def_buffer([](TPXFrame &m) -> py::buffer_info {
    //         return py::buffer_info(
    //             m.hits(),                               /* Pointer to buffer */
    //             sizeof(TPXHit),                         /* Size of one scalar */
    //             py::format_descriptor<TPXHit>::format(), /* Python struct-style format descriptor */
    //             1,                                      /* Number of dimensions */
    //             { m.n_hits() },                         /* Buffer dimensions */
    //             { sizeof(TPXHit) }             /* Strides (in bytes) for each index */
    //         );
    //     });
    // // frame.def()

}
