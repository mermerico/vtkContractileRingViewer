#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkStructuredPointsReader.h>
#include <vtkSLCReader.h>
#include <vtkStructuredPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkThreshold.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkStdString.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTesting.h>
#include <vtkTIFFReader.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkParametricTorus.h>
#include <vtkParametricFunctionSource.h>
#include <vtkBoxWidget.h>
#include <vtkCommand.h>
#include <vtkTransform.h>
#include <vtkProperty.h>
#include <vtkSelectEnclosedPoints.h>


class vtkMyCallback : public vtkCommand
{
public:
  static vtkMyCallback *New()
    { return new vtkMyCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkTransform *t = vtkTransform::New();
      vtkBoxWidget *widget = reinterpret_cast<vtkBoxWidget*>(caller);
      widget->GetTransform(t);
      widget->GetProp3D()->SetUserTransform(t);
      t->Delete();
    }
};

int main(int argc, char*argv[])
{
  if(argc < 2)
  {
    std::cerr << "Required arguments: imagePrefix" << std::endl;
    return EXIT_FAILURE;
  }

  // Create the standard renderer, render window, and interactor.
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  iren->SetInteractorStyle(style);

  // Create the reader for the data.
  vtkSmartPointer<vtkTIFFReader> reader =
    vtkSmartPointer<vtkTIFFReader>::New();
  reader->SetFilePrefix(argv[1]);
  reader->SetFilePattern("%s%.2d.tif");
  //reader->setFileName(argv[1]);
  reader->SetDataExtent(0, 22, 0, 28, 0, 18);
  reader->SetDataOrigin(0.0, 0.0, 0.0);
  reader->SetDataScalarTypeToUnsignedShort();
  reader->SetDataByteOrderToLittleEndian();
  reader->UpdateWholeExtent();
  reader->SetDataSpacing(0.053750, 0.053750, 10*(6.12/17)/0.133);
  double x,y,z;
  reader->GetDataSpacing(x,y,z);
  cout << "maxX:" << x << " maxY:" << y << " maxZ:" << z << endl;
  reader->Update();
  reader->GetDataSpacing(x,y,z);
  cout << "maxX:" << x << " maxY:" << y << " maxZ:" << z << endl;

  vtkSmartPointer<vtkParametricTorus> torus =
    vtkSmartPointer<vtkParametricTorus>::New();
  vtkSmartPointer<vtkParametricFunctionSource> funcSource =
    vtkSmartPointer<vtkParametricFunctionSource>::New();
  funcSource->SetParametricFunction(torus);
  funcSource->SetUResolution(25);
  funcSource->SetVResolution(25);
  funcSource->SetWResolution(25);
  funcSource->Update();


  // Convert from vtkImageData to vtkUnstructuredGrid.
  // Remove any cells where all values are below 80.
  /*vtkSmartPointer<vtkThreshold> thresh =
    vtkSmartPointer<vtkThreshold>::New();
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // Make sure we have only tetrahedra.
  vtkSmartPointer<vtkDataSetTriangleFilter> trifilter =
    vtkSmartPointer<vtkDataSetTriangleFilter>::New();
  trifilter->SetInputConnection(thresh->GetOutputPort());*/

  // Create transfer mapping scalar value to opacity.
  vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  opacityTransferFunction->AddPoint(10*80.0,  0.0);
  opacityTransferFunction->AddPoint(50*80.0, 0.0);
  opacityTransferFunction->AddPoint(50*255.0, 0.4);

  // Create transfer mapping scalar value to color.
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddRGBPoint(50*80.0,  0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(50*120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(50*160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(50*200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(50*255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look.
  vtkSmartPointer<vtkVolumeProperty> volumeProperty =
    vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper =
    vtkSmartPointer<vtkSmartVolumeMapper>::New();
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  //volumeMapper->Update();
  double bounds[6];
  /*volumeMapper->GetBounds(bounds);
  for(int i = 0; i < 6; i++)
      cout << bounds[i] << " ";
  cout << endl;*/

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  vtkSmartPointer<vtkVolume> volume =
    vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  reader->GetDataSpacing(x,y,z);
  cout << "maxX:" << x << " maxY:" << y << " maxZ:" << z << endl;
  cout << "maxX:" << volume->GetMaxXBound() << " maxY:" << volume->GetMaxYBound() << " maxZ:" << volume->GetMaxZBound() << endl;

  // Contour the second dataset.
  /*vtkSmartPointer<vtkContourFilter> contour =
    vtkSmartPointer<vtkContourFilter>::New();
  contour->SetValue(0, 80);
  contour->SetInputConnection(reader2->GetOutputPort());*/

  // Create a mapper for the polygonal data.
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(funcSource->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create an actor for the polygonal data.
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  //actor->GetProperty()->SetOpacity(0.8);
  actor->GetProperty()->SetColor(1,1,1);
  actor->GetProperty()->SetRepresentationToWireframe();
  cout << "maxX:" << volume->GetMaxXBound() << " maxY:" << volume->GetMaxYBound() << " maxZ:" << volume->GetMaxZBound() << endl;



  // Here we use a vtkBoxWidget to transform the underlying coneActor (by
  // manipulating its transformation matrix). Many other types of widgets
  // are available for use, see the documentation for more details.
  //
  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()). The place factor
  // controls the initial size of the widget with respect to the bounding box
  // of the input to the widget.
  vtkBoxWidget *boxWidget = vtkBoxWidget::New();
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);

  //
  // Place the interactor initially. The input to a 3D widget is used to
  // initially position and scale the widget. The EndInteractionEvent is
  // observed which invokes the SelectPolygons callback.
  //
  boxWidget->SetProp3D(actor);
  boxWidget->PlaceWidget();
  vtkMyCallback *callback = vtkMyCallback::New();
  boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);

  double xSize = volume->GetMaxXBound();
  double ySize = volume->GetMaxYBound();
  double zSize = volume->GetMaxZBound();
  vtkTransform *t = vtkTransform::New();
  t->Identity();
  t->PostMultiply();
  t->RotateY(90);
  t->Scale(xSize/8,ySize/8,zSize/8);
  t->Translate(xSize/2,ySize/2,zSize/2);
  boxWidget->SetTransform(t);
  boxWidget->GetProp3D()->SetUserTransform(t);

  //
  // Normally the user presses the "i" key to bring a 3D widget to life. Here
  // we will manually enable it so it appears with the cone.
  //
  boxWidget->On();

  // First test if mapper is supported
  renWin->SetSize(600, 600);

  ren1->AddViewProp(volume);
  ren1->AddViewProp(actor);

  ren1->ResetCamera();
  ren1->GetActiveCamera()->Azimuth(20.0);
  ren1->GetActiveCamera()->Elevation(10.0);
  ren1->GetActiveCamera()->Zoom(1.5);
  ren1->SetBackground(0.3,0.3,0.3);

  // Test default settings
  renWin->Render();

  iren->Start();

  vtkLinearTransform* t_out;
  t_out = boxWidget->GetProp3D()->GetUserTransform();
  vtkAbstractTransform* inverseTransform = t_out->GetInverse();
  inverseTransform->Print(cout);
  t_out->Print(cout);
  vtkSmartPointer<vtkSelectEnclosedPoints> enclosedPts = vtkSmartPointer<vtkSelectEnclosedPoints>::New();
  //enclosedPts->SetSurface(funcSource->GetOutput());
  //enclosedPts->Update();
  enclosedPts->Initialize(funcSource->GetOutput());
  cout <<" 000:" << enclosedPts->IsInsideSurface(0,0.5,0) << endl;
  double posInCoord[3] = {0.0,0.5,0};
  double posTransCoord[3];
  t_out->TransformPoint(posInCoord,posTransCoord);
  cout << "transformed:" << posTransCoord[0] << " " << posTransCoord[1] <<  " " << posTransCoord[2] << endl;
  double sum = 0;
  int numPts = 0;
  for(int z = 0; z < 18; z++)
      for(int y = 0; y < 28; y++)
          for(int x = 0; x < 22; x++)
          {
              double posViewerCoord[3] = {10*x, 10*y,z*10*(6.12/17)/0.133};
              double xViewer = posViewerCoord[0];
              double yViewer = posViewerCoord[1];
              double zViewer = posViewerCoord[2];
              double posTorusCoord[3];
              inverseTransform->TransformPoint(posViewerCoord,posTorusCoord);
              double xTorus = posTorusCoord[0];
              double yTorus = posTorusCoord[1];
              double zTorus = posTorusCoord[2];
              if(x == 11 && y == 14 && z == 9)
              {
                  cout << "xViewer: " << xViewer << " yViewer: " << yViewer << " zViewer: " << zViewer << endl;
                  cout << "xTorus: " << xTorus << " yTorus" << yTorus << " zTorus" << zTorus << endl;
              }
              if(enclosedPts->IsInsideSurface(xTorus,yTorus,zTorus))
              {
                  unsigned short* ptr = static_cast<unsigned short*>(reader->GetOutput()->GetScalarPointer(x,y,z));
                  sum += *ptr;
                  numPts++;
              }
          }
    cout << " sum pixels: " << sum << " numPts: " << numPts << endl;
  return EXIT_SUCCESS;
}
