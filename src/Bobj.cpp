#include "Bobj.hpp"
#include <mars_utils/misc.h>

using namespace std;

namespace mars
{
    using namespace utils;
    using namespace interfaces;

    namespace vsg_graphics
    {
        vsg::ref_ptr<vsg::Node> Bobj::readFromFile(const std::string &filename)
        {
            FILE* input = fopen(filename.c_str(), "rb");
            if(!input)
            {
                LOG_ERROR("Bobj::readFromFile: reading file: %s\n", filename.c_str());
                return 0;
            }
            char buffer[312];

            int da, r, o, foo=0;
            size_t i;
            int iData[3];
            float fData[4];

            std::vector<vsg::vec3> vertices;
            std::vector<vsg::vec3> normals;
            std::vector<vsg::vec2> texcoords;
            std::vector<vsg::vec4> colors;
            std::vector<unsigned short> indices;

            std::vector<vsg::vec3> vertices2;
            std::vector<vsg::vec3> normals2;
            std::vector<vsg::vec2> texcoords2;
            std::vector<vsg::vec4> colors2;
            std::vector<unsigned short> indices2;

            bool useIndices = true;
            int indicesCount = 0;
            while((r = fread(buffer+foo, 1, 256, input)) > 0 )
            {
                o = 0;
                while(o < r+foo-50 || (r<256 && o < r+foo))
                {
                    memcpy(&da, buffer+o, sizeof(int));
                    //da = *(int*)(buffer+o);
                    o += 4;
                    if(da == 1)
                    {
                        for(i=0; i<3; i++)
                        {
                            memcpy(fData+i, buffer+o, sizeof(float));
                            //fData[i] = *(float*)(buffer+o);
                            o+=4;
                        }
                        vertices.push_back(vsg::vec3(fData[0], fData[1], fData[2]));
                    }
                    else if(da == 2)
                    {
                        for(i=0; i<2; i++)
                        {
                            memcpy(fData+i, buffer+o, sizeof(float));
                            //fData[i] = *(float*)(buffer+o);
                            o+=4;
                        }
                        texcoords.push_back(vsg::vec2(fData[0], fData[1]));
                    }
                    else if(da == 3)
                    {
                        for(i=0; i<3; i++)
                        {
                            memcpy(fData+i, buffer+o, sizeof(float));
                            //fData[i] = *(float*)(buffer+o);
                            o+=4;
                        }
                        normals.push_back(vsg::vec3(fData[0], fData[1], fData[2]));
                    }
                    else if(da == 4)
                    {
                        // 1. vertice
                        for(i=0; i<3; i++)
                        {
                            memcpy(iData+i, buffer+o, sizeof(int));
                            //iData[i] = *(int*)(buffer+o);
                            o+=4;
                        }
                        if(iData[0] != iData[2])
                        {
                            useIndices = false;
                        }
                        // add vsg vertices etc.
                        indices.push_back(iData[0]-1);
                        vertices2.push_back(vertices[iData[0]-1]);
                        indices2.push_back(indicesCount++);
                        if(colors.size() == vertices.size())
                        {
                            colors2.push_back(colors[iData[0]-1]);
                        }
                        if(iData[1] > 0)
                        {
                            texcoords2.push_back(texcoords[iData[1]-1]);
                        }
                        normals2.push_back(normals[iData[2]-1]);

                        // 2. vertice
                        for(i=0; i<3; i++)
                        {
                            memcpy(iData+i, buffer+o, sizeof(int));
                            //iData[i] = *(int*)(buffer+o);
                            o+=4;
                        }
                        if(iData[0] != iData[2])
                        {
                            useIndices = false;
                        }
                        indices.push_back(iData[0]-1);
                        // add vsg vertices etc.
                        vertices2.push_back(vertices[iData[0]-1]);
                        indices2.push_back(indicesCount++);
                        if(colors.size() == vertices.size())
                        {
                            colors2.push_back(colors[iData[0]-1]);
                        }
                        if(iData[1] > 0)
                        {
                            texcoords2.push_back(texcoords[iData[1]-1]);
                        }
                        normals2.push_back(normals[iData[2]-1]);

                        // 3. vertice
                        for(i=0; i<3; i++)
                        {
                            memcpy(iData+i, buffer+o, sizeof(int));
                            //iData[i] = *(int*)(buffer+o);
                            o+=4;
                        }
                        if(iData[0] != iData[2])
                        {
                            useIndices = false;
                        }
                        indices.push_back(iData[0]-1);
                        // add vsg vertices etc.
                        vertices2.push_back(vertices[iData[0]-1]);
                        indices2.push_back(indicesCount++);
                        if(colors.size() == vertices.size())
                        {
                            colors2.push_back(colors[iData[0]-1]);
                        }
                        if(iData[1] > 0)
                        {
                            texcoords2.push_back(texcoords[iData[1]-1]);
                        }
                        normals2.push_back(normals[iData[2]-1]);
                    }
                    else if(da == 5)
                    { // vertex colors
                        for(i=0; i<4; i++)
                        {
                            memcpy(fData+i, buffer+o, sizeof(float));
                            //fData[i] = *(float*)(buffer+o);
                            o+=4;
                        }
                        // debug indices:
                        // float index = (fData[0]* 0.996108955f + fData[1]*0.003891045f)*65535.0f;
                        // fprintf(stderr, " %ld->%ld", colors.size(), lroundf(index));
                        colors.push_back(vsg::vec4(fData[0], fData[1], fData[2], fData[3]));
                    }
                }
                foo = r+foo-o;
                if(r==256) memcpy(buffer, buffer+o, foo);
            }
            //fprintf(stderr, "\n");

            vsg::ref_ptr<vsg::vec3Array> vsgVertices;
            vsg::ref_ptr<vsg::vec2Array> vsgTexcoords;
            vsg::ref_ptr<vsg::vec3Array> vsgNormals;
            vsg::ref_ptr<vsg::vec4Array> vsgColors;
            vsg::ref_ptr<vsg::ushortArray> vsgIndices;
            if(useIndices)
            {
                vsgVertices = new vsg::vec3Array(vertices.size());
                vsgTexcoords = new vsg::vec2Array(texcoords.size());
                vsgNormals = new vsg::vec3Array(normals.size());
                vsgColors = new vsg::vec4Array(colors.size());
                vsgIndices = new vsg::ushortArray(indices.size());
                auto itr = vsgIndices->begin();
                for(i=0; i<indices.size(); ++i)
                {
                    (*itr++) = indices[i];
                }
                for(i=0; i<vertices.size(); ++i)
                {
                    vsgVertices->at(i) = vertices[i];
                    vsgNormals->at(i) = normals[i];
                    if(colors.size() > 0)
                    {
                        vsgColors->at(i) = colors[i];
                    }
                    if(texcoords.size() > i)
                    {
                        vsgTexcoords->at(i) = texcoords[i];
                    }
                }
            }
            else
            {
                vsgVertices = new vsg::vec3Array(vertices2.size());
                vsgTexcoords = new vsg::vec2Array(texcoords2.size());
                vsgNormals = new vsg::vec3Array(normals2.size());
                vsgColors = new vsg::vec4Array(colors2.size());
                vsgIndices = new vsg::ushortArray(indices2.size());
                std::memcpy(vsgVertices->dataPointer(), vertices2.data(), vertices2.size() * 12);
                std::memcpy(vsgNormals->dataPointer(), normals2.data(), normals2.size() * 12);
                std::memcpy(vsgIndices->dataPointer(), indices2.data(), indices2.size() * 2);
                std::memcpy(vsgTexcoords->dataPointer(), texcoords2.data(), texcoords2.size() * 8);
                std::memcpy(vsgColors->dataPointer(), colors2.data(), colors2.size() * 16);
            }

            auto node = vsg::VertexIndexDraw::create();

            vsg::DataList arrays;
            arrays.push_back(vsgVertices);
            if (vsgNormals->size()) arrays.push_back(vsgNormals);
            if (vsgTexcoords->size()) arrays.push_back(vsgTexcoords);
            if (vsgColors->size()) arrays.push_back(vsgColors);
            // LOG_ERROR("num vertices: %lu", vsgVertices->size());
            // LOG_ERROR("num indices: %lu", vsgIndices->size());
            // LOG_ERROR("num normals: %lu", vsgNormals->size());
            // LOG_ERROR("num texcoords: %lu", vsgTexcoords->size());
            // LOG_ERROR("num colors: %lu", vsgColors->size());
            node->assignArrays(arrays);
            node->assignIndices(vsgIndices);
            node->indexCount = static_cast<uint32_t>(vsgIndices->size());
            node->instanceCount = 1;

            return node;
        }

        bool Bobj::checkBobj(std::string &filename)
        {
            string tmpfilename, tmpfilename2;
            tmpfilename = filename;
            std::string suffix = getFilenameSuffix(filename);
            if(suffix != ".bobj")
            {
                // turn our relative filename into an absolute filename
                removeFilenameSuffix(&tmpfilename);
                tmpfilename.append(".bobj");
                // replace if that file exists
                if (pathExists(tmpfilename))
                {
                    //fprintf(stderr, "Loading .bobj instead of %s for file: %s\n", suffix.c_str(), tmpfilename.c_str());
                    filename = tmpfilename;
                }
                else
                {
                    // check if bobj files are in parallel folder
                    std::vector<std::string> arrayPath = explodeString('/', tmpfilename);
                    tmpfilename = "";
                    for(size_t i=0; i<arrayPath.size(); ++i)
                    {
                        if(i==0)
                        {
                            if(arrayPath[i] == "")
                            {
                                tmpfilename = "/";
                            }
                            else
                            {
                                tmpfilename = arrayPath[i];
                            }
                        }
                        else if(i==arrayPath.size()-2)
                        {
                            tmpfilename = pathJoin(tmpfilename, "bobj");
                        }
                        else
                        {
                            tmpfilename = pathJoin(tmpfilename, arrayPath[i]);
                        }
                    }
                    if (pathExists(tmpfilename))
                    {
                        //fprintf(stderr, "Loading .bobj instead of %s for file: %s\n", suffix.c_str(), tmpfilename.c_str());
                        filename = tmpfilename;
                        return true;
                    }
                }
                return false;
            }
            return true;
        }

    } // end of namespace vsg_graphics
} // end of namespace mars
