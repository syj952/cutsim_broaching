#pragma once

#include <windows.h>
#include <chrono> // For timing

#include "src/cutsim/cutsim/gldata.hpp"

// OCCT Includes
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_Group.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_Presentation.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>
#include "Viewer.h"
#include "ViewerInteractor.h"
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Vertex.hxx>
#include <AIS_Shape.hxx>
namespace cutsim {

    class AIS_CutSimMesh : public AIS_InteractiveObject
    {
    public:
        AIS_CutSimMesh(GLData* glData) : myGLData(glData) {}
        Handle(Graphic3d_ArrayOfTriangles) aTriangles;
        virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
            const Handle(Prs3d_Presentation)& thePrs,
            const Standard_Integer theMode) override
        {
            auto start = std::chrono::high_resolution_clock::now();

            (void)thePrsMgr;
            (void)theMode;

            if (!myGLData)
                return;

            QMutexLocker locker(&(myGLData->renderMutex));
            if (myGLData->vertexCount() == 0)
                return;

            const GLVertex* vertices = myGLData->getVertexArray();
            const unsigned int* indices = myGLData->getIndexArray();
            int numVertices = myGLData->vertexCount();
            int numIndices = myGLData->indexCount();
            std::cout << "三角面片数量 " << numVertices / 3 << std::endl;
            //aTriangles.reset()
            aTriangles = new Graphic3d_ArrayOfTriangles(
                numVertices,
                numIndices,
                Standard_True,// hasNormals
                Standard_True  // hasColors
            );
            float minNormal = 1.0f, maxNormal = 0.0f;

            // Add Vertices
            for (int i = 0; i < numVertices; ++i)
            {
                const GLVertex& v = vertices[i];
                //aTriangles->AddVertex(v.x, v.y, v.z,
                //    v.nx, v.ny, v.nz);
                //aTriangles->SetVertexColor(i + 1, v.r, v.g, v.b);
                // 检查法向量长度
                float normalLength = sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
                if (normalLength < 0.01f) {
                    // 法向量太短，使用默认向上法向量
                    aTriangles->AddVertex(v.x, v.y, v.z, 0.0f, 0.0f, 1.0f);
                }
                else {
                    // 单位化法向量
                    aTriangles->AddVertex(v.x, v.y, v.z,
                        v.nx / normalLength,
                        v.ny / normalLength,
                        v.nz / normalLength);
                }

                // 增强颜色亮度
                float brightnessBoost = 1.5f;  // 增加50%亮度
                float r = qMin(v.r * brightnessBoost, 1.0f);
                float g = qMin(v.g * brightnessBoost, 1.0f);
                float b = qMin(v.b * brightnessBoost, 1.0f);

                aTriangles->SetVertexColor(i + 1, r, g, b);

            }
            Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
            Handle(Prs3d_ShadingAspect) aShadingAspect = new Prs3d_ShadingAspect();
            //// 重要：设置更亮的材质属性
            //Handle(Graphic3d_AspectFillArea3d) anAspect = new Graphic3d_AspectFillArea3d();

            //// 1. 设置材质反射属性
            //Graphic3d_MaterialAspect *aMaterial = new Graphic3d_MaterialAspect();

            //// 使用更亮的材质类型
            //aMaterial->SetMaterialType(Graphic3d_MATERIAL_ASPECT);
            //aMaterial->SetAmbientColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));  // 环境光反射
            //aMaterial->SetDiffuseColor(Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB));  // 漫反射
            //aMaterial->SetSpecularColor(Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB)); // 高光反射
            //aMaterial->SetEmissiveColor(Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB)); // 自发光
            //aMaterial->SetShininess(0.3f);  // 光泽度
            //aMaterial->SetTransparency(0.0f); // 不透明

            //anAspect->SetFrontMaterial(*aMaterial);
            //anAspect->SetBackMaterial(*aMaterial);

            //// 2. 设置多边形填充模式
            //anAspect->SetInteriorStyle(Aspect_IS_SOLID);
            //anAspect->SetEdgeOff();  // 关闭边线显示

            //aGroup->SetGroupPrimitivesAspect(anAspect);
            //aGroup->AddPrimitiveArray(aTriangles);

            aShadingAspect->SetColor(Quantity_NOC_GRAY70);
            aGroup->SetGroupPrimitivesAspect(aShadingAspect->Aspect());
            aGroup->AddPrimitiveArray(aTriangles);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            std::cout << "[Profile] AIS_CutSimMesh::Compute took " << duration.count() << " ms" << std::endl;
        }

        virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
            const Standard_Integer theMode) override
        {
            // Selection not implemented
            (void)theSelection;
            (void)theMode;
        }

    private:
        GLData* myGLData;
    };

    class OcctViewer
    {
    public:
        static LRESULT CALLBACK HandleWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_DESTROY)
            {
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        static void Show(GLData* glData)
        {
            auto start = std::chrono::high_resolution_clock::now();

            Viewer vout(200, 200, 600, 600);
            // 7. Display Mesh
            Handle(AIS_CutSimMesh) aMesh = new AIS_CutSimMesh(glData);
            vout.AddShape(aMesh);
            aMesh->SetColor(Quantity_NOC_WHITE);

            // 8. Message Loop
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            std::cout << "[Profile] OcctViewer::Show init took " << duration.count() << " ms" << std::endl;
            vout.StartMessageLoop();
        }
        static Handle(AIS_InteractiveObject) getGraphic3d(GLData* glData)
        {
            auto start = std::chrono::high_resolution_clock::now();
            Handle(AIS_CutSimMesh) aMesh = new AIS_CutSimMesh(glData);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            std::cout << "[Profile] OcctViewer transform init took " << duration.count() << " ms" << std::endl;
            return aMesh;
        }
    };

} // namespace cutsim
