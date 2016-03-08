#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct Rectangle
{
    float x,y=0,z;
    VAO *rectangle;
};
typedef struct Rectangle Rectangle;
Rectangle Rectangles[1000];

struct Obstacle 
{
    float x,y,z;
    VAO *obstacle;
};
typedef struct Obstacle Obstacle;
Obstacle Obstacles[1000];
GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float player_rot = 0, dir = 1;
float x = -7, z = 6, y = 2, t = 0.1;
float camera_x = 1, camera_y = 5, camera_z = 0;
float angle_xz = 90, camera_dist_xz = 10, camera_dist_yz = 7, angle_yz = 0;
int state1 = 0, state2 = 0, state3 = 0, state4 = 0, state5 = 0, state6 = 0, state7 = 0, state8 = 0, space = 0;
int view = 0;
int b[500]={0};
double xpos, ypos;
float zoom=1;
int a[500]={0};
int st = 0, st1 = 0;
int t1 = 0;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            case GLFW_KEY_UP:
                state1 = 0;
                break;
            case GLFW_KEY_DOWN:
                state2 = 0;
                break;
            case GLFW_KEY_LEFT:
                state3 = 0;
                break;
            case GLFW_KEY_RIGHT:
                state4 = 0;
                break;
            case GLFW_KEY_F:
                t+=0.2;
                if(t>=0.2)
                  t=0.2;
                break;
            case GLFW_KEY_S:
                t-=0.2;
                if(t<=0.5)
                  t=0.2;
                break;
            case GLFW_KEY_W:
                //angle_xz++;
                state5 = 0;
                break;
            case GLFW_KEY_E:
                //angle_xz--;
                state6 = 0;
                break;
            case GLFW_KEY_D:
                state7 = 0;
                //camera_y-=0.2;
                break;
            case GLFW_KEY_A:
                state8 = 0;
                //camera_y+=0.2;
                break;
            case GLFW_KEY_SPACE:
                space = 1;
                break;
            case GLFW_KEY_1:
                view=1;
                break;
            case GLFW_KEY_2:
                view=2;
                break;
            case GLFW_KEY_3:
                view=3;
                break;
            case GLFW_KEY_4:
                view=4;
                break;
            case GLFW_KEY_5:
                view=5;
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_UP:
                state1 = 1;
                break;
            case GLFW_KEY_DOWN:
                state2 = 1;
                break;
            case GLFW_KEY_LEFT:
                state3 = 1;
                break;
            case GLFW_KEY_RIGHT:
                state4 = 1;
                break;
            case GLFW_KEY_W:
                //angle_xz++;
                state5 = 1;
                break;
            case GLFW_KEY_E:
                //angle_xz--;
                state6 = 1;
                break;
            case GLFW_KEY_D:
                state7 = 1;
                //camera_y-=0.2;
                break;
            case GLFW_KEY_A:
                state8 = 1;
                //camera_y+=0.2;
                break;            
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, 0.1f, 500.0f);
}

VAO *triangle, *player;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createObstacle ()
{
    static const GLfloat vertex_buffer_data [] = {
        -0.25,0,0.25, // vertex 1
        0.25,0,0.25, // vertex 2
        0.25, 0,-0.25, // vertex 3

        0.25, 0,-0.25, // vertex 3
        -0.25, 0,-0.25, // vertex 4
        -0.25,0,0.25,  // vertex 1
        
        -0.25,0,0.25,
        -0.25,-0.5,0.25,
        0.25,-0.5,0.25,
        
        0.25,-0.5,0.25,
        0.25,0,0.25,
        -0.25,0,0.25,

        0.25,0,0.25,
        0.25,-0.5,0.25,
        0.25,-0.5,-0.25,

        0.25,-0.5,-0.25,
        0.25,0,-0.25,
        0.25,0,0.25,

        0.25,0,-0.25,
        0.25,-0.5,-0.25, 
        -0.25, -0.5,-0.25, 

        -0.25, -0.5,-0.25, 
        -0.25, 0,-0.25,
        0.25,0,-0.25,

        -0.25,0,-0.25,
        -0.25,-0.5,-0.25,
        -0.25,-0.5,0.25,

        -0.25,-0.5,0.25,
        -0.25,0,0.25,
        -0.25,0,-0.25,

        0.25,-0.5,0.25,
        0.25,-0.5,-0.25,
        -0.25,-0.5,-0.25,
        
        -0.25,-0.5,-0.25,
        -0.25,-0.5,0.25,
        0.25,-0.5,0.25
    
    };

    static const GLfloat color_buffer_data [] = {
        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,
        
        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,
        
        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,
        
        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,

        1,1,1,
        1,1,1,
        1,1,1,
        
        1,1,1,
        1,1,1,
        1,1,1
    };
    for(int i=0;i<200;i++)
    {
        if(b[i]==1)
            Obstacles[i].obstacle = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
    }

}
// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.5,0,0.5, // vertex 1
    0.5,0,0.5, // vertex 2
    0.5, 0,-0.5, // vertex 3

    0.5, 0,-0.5, // vertex 3
    -0.5, 0,-0.5, // vertex 4
    -0.5,0,0.5,  // vertex 1
    
    -0.5,0,0.5,
    -0.5,-2,0.5,
    0.5,-2,0.5,
    
    0.5,-2,0.5,
    0.5,0,0.5,
    -0.5,0,0.5,

    0.5,0,0.5,
    0.5,-2,0.5,
    0.5,-2,-0.5,

    0.5,-2,-0.5,
    0.5,0,-0.5,
    0.5,0,0.5,

    0.5,0,-0.5,
    0.5,-2,-0.5, 
    -0.5, -2,-0.5, 

    -0.5, -2,-0.5, 
    -0.5, 0,-0.5,
    0.5,0,-0.5,

    -0.5,0,-0.5,
    -0.5,-2,-0.5,
    -0.5,-2,0.5,

    -0.5,-2,0.5,
    -0.5,0,0.5,
    -0.5,0,-0.5,

    0.5,-2,0.5,
    0.5,-2,-0.5,
    -0.5,-2,-0.5,
    
    -0.5,-2,-0.5,
    -0.5,-2,0.5,
    0.5,-2,0.5

  };

  GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1

    1,0,0, 
    0,0,1, 
    0,1,0, 

    0,1,0, 
    0.3,0.3,0.3, 
    1,0,0,  

    1,0,0, 
    0,0,1, 
    0,1,0, 

    0,1,0, 
    0.3,0.3,0.3, 
    1,0,0,  

    1,0,0, 
    0,0,1, 
    0,1,0, 

    0,1,0, 
    0.3,0.3,0.3, 
    1,0,0,  

    1,0,0, 
    0,0,1, 
    0,1,0, 

    0,1,0, 
    0.3,0.3,0.3, 
    1,0,0,  

    1,0,0, 
    0,0,1, 
    0,1,0, 

    0,1,0, 
    0.3,0.3,0.3, 
    1,0,0  
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  for(int i=0;i<1000;i++)
  {
    if (i==13)
    {        
        for(int j=0;j<=17;)
        {
            color_buffer_data[j++]=153;
            color_buffer_data[j++]=0;
            color_buffer_data[j++]=153;
        }
    }
    else
    {
        color_buffer_data[0]=1;
        color_buffer_data[1]=0;
        color_buffer_data[2]=0;
        color_buffer_data[3]=0;
        color_buffer_data[4]=0;
        color_buffer_data[5]=1;
        color_buffer_data[6]=0;
        color_buffer_data[7]=1;
        color_buffer_data[8]=0;
        color_buffer_data[9]=0;
        color_buffer_data[10]=1;
        color_buffer_data[11]=0;
        color_buffer_data[12]=0.3;
        color_buffer_data[13]=0.3;
        color_buffer_data[14]=0.3;
        color_buffer_data[15]=1;
        color_buffer_data[16]=0;
        color_buffer_data[17]=0;
    }
    Rectangles[i].rectangle = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
  }
}

void createPlayer ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.25,0,0.25, // vertex 1
    0.25,0,0.25, // vertex 2
    0.25, 0,-0.25, // vertex 3

    0.25, 0,-0.25, // vertex 3
    -0.25, 0,-0.25, // vertex 4
    -0.25,0,0.25,  // vertex 1
    
    -0.25,0,0.25,
    -0.25,-1,0.25,
    0.25,-1,0.25,
    
    0.25,-1,0.25,
    0.25,0,0.25,
    -0.25,0,0.25,

    0.25,0,0.25,
    0.25,-1,0.25,
    0.25,-1,-0.25,

    0.25,-1,-0.25,
    0.25,0,-0.25,
    0.25,0,0.25,

    0.25,0,-0.25,
    0.25,-1,-0.25, 
    -0.25, -1,-0.25, 

    -0.25, -1,-0.25, 
    -0.25, 0,-0.25,
    0.25,0,-0.25,

    -0.25,0,-0.25,
    -0.25,-1,-0.25,
    -0.25,-1,0.25,

    -0.25,-1,0.25,
    -0.25,0,0.25,
    -0.25,0,-0.25,

    0.25,-1,0.25,
    0.25,-1,-0.25,
    -0.25,-1,-0.25,
    
    -0.25,-1,-0.25,
    -0.25,-1,0.25,
    0.25,-1,0.25

  };

  static const GLfloat color_buffer_data [] = {
    76,153,0, 
    76,153,0, 
    76,153,0, 

    76,153,0, 
    76,153,0, 
    76,153,0,

    0,51,102, 
    0,51,102, 
    0,51,102,
    
    0,51,102, 
    0,51,102, 
    0,51,102,  

    255,51,153, 
    255,51,153, 
    255,51,153, 

    255,51,153, 
    255,51,153, 
    255,51,153,  

    51,0,102, 
    51,0,102, 
    51,0,102, 

    51,0,102, 
    51,0,102, 
    51,0,102,

    255,0,0, 
    255,0,0, 
    255,0,0, 

    255,0,0, 
    255,0,0, 
    255,0,0,

    0,255,0, 
    0,255,0, 
    0,255,0, 

    0,255,0, 
    0,255,0, 
    0,255,0  
  };

  // create3DObject creates and returns a handle to a VAO that can be used later

  player = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);
  //if(view==1)
  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  if(view==1)
  {
    //Matrices.projection = glm::ortho(-4.0f, 4.0f, -2.25f, 2.25f, 0.0f, 500.0f);
    //if(dir==1)
    //{
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -2.25f, 2.25f, 0.0f, 500.0f);
        glm::vec3 eye (x,5,z);
        glm::vec3 target(x+8,-2,z-8);
        Matrices.view = glm::lookAt(eye, target, glm::vec3(0,1,0));
    //}
        /*
    else if(dir==2)
    {
        glm::vec3 eye (x,2,z);
        glm::vec3 target(x,1,z+2);
        Matrices.view = glm::lookAt(eye, target, glm::vec3(0,1,0));   
    }
    else if(dir==3)
    {
        glm::vec3 eye (x,2,z);
        glm::vec3 target(x-2,1,z);
        Matrices.view = glm::lookAt(eye, target, glm::vec3(0,1,0));
    }
    else if(dir==4)
    {
        glm::vec3 eye (x,2,z+3);
        glm::vec3 target(x,1,z-2);
        Matrices.view = glm::lookAt(eye, target, glm::vec3(0,1,0));
    }*/
  }
  else if(view==2)
  {
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -2.25f, 2.25f, 0.0f, 500.0f);
    glm::vec3 eye (x,2,z+2);
    glm::vec3 target(x,1,z);
    Matrices.view = glm::lookAt(eye, target, glm::vec3(0,1,0));
  }
  //cout<<dir<<endl;
  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  /*if(view==1)
  {
    if(dir==1)
        Matrices.view = glm::lookAt(glm::vec3(x,y,z), glm::vec3(1,0,0), glm::vec3(0,1,0));
    else if(dir==2)
        Matrices.view = glm::lookAt(glm::vec3(x,y,z), glm::vec3(0,0,-1), glm::vec3(0,1,0));
    else if(dir==3)
        Matrices.view = glm::lookAt(glm::vec3(x,y,z), glm::vec3(-1,0,0), glm::vec3(0,1,0));
    else if(dir==4)
        Matrices.view = glm::lookAt(glm::vec3(x,y,z), glm::vec3(0,0,1), glm::vec3(0,1,0));
  }*/
  else if(view==3)
  {
    Matrices.projection = glm::ortho(-12.0f, 12.0f, -7.0f, 7.0f, 0.1f, 500.0f);
    Matrices.view = glm::lookAt(glm::vec3(6,10,6),glm::vec3(0,0,0),glm::vec3(0,1,0));
  }
  else if(view==4)
  {
    Matrices.projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, 0.1f, 500.0f);
    Matrices.view = glm::lookAt(glm::vec3(0,10,0), glm::vec3(0,0,0), glm::vec3(0,0,-1)); 
  }
  else if (view==5)
  {
    Matrices.projection = glm::ortho(-16.0f*zoom, 16.0f*zoom, -9.0f*zoom, 9.0f*zoom, 0.1f, 500.0f);
    Matrices.view = glm::lookAt(glm::vec3(xpos,ypos,10), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  }
  else
  {
    Matrices.projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, 0.1f, 500.0f);
    Matrices.view = glm::lookAt(glm::vec3(10,5,10),glm::vec3(0,0,0),glm::vec3(0,1,0));
  }
  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  //Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

  //glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  //glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  //Matrices.model *= triangleTransform; 
  //MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  //glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  //draw3DObject(triangle);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatePlayer = glm::translate (glm::vec3(x, y-1, z));        // glTranslatef
  glm::mat4 rotatePlayer = glm::rotate((float)(player_rot*M_PI/180.0f), glm::vec3(0,1,0)); // rotate about vector (-1,1,1)
  Matrices.model = (translatePlayer * rotatePlayer);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(player);
  
  for(int k=-7,i=0;k<7;k++)
  {
    
    for(int j=-7;j<7;j++)
    {
      
      if(((j==2)&&(k==-1))||((j==0)&&(k==1))||((j==-3)&&(k==3))||((j==-2)&&(k==5))||((j==4)&&(k==-4))||((j==-2)&&(k==-1)));
      else
      {
        Matrices.model = glm::mat4(1.0f);
        Rectangles[i].x=j;
        Rectangles[i].z=k;
        glm::mat4 translateRectangle = glm::translate (glm::vec3(j, Rectangles[i].y, k));        // glTranslatef
        //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
        Matrices.model = translateRectangle;
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(Rectangles[i].rectangle);
        i++;
        //cout<<j<<" "<<k<<endl;

      }
    }
  } 
  for(int i=0;i<200;i++)
  {
    if(b[i]==1)
    {
        Obstacles[i].x=Rectangles[i].x;
        Obstacles[i].z=Rectangles[i].z;
        Obstacles[i].y=0.5;
        Matrices.model = glm::mat4(1.0f);
        glm::mat4 translateRectangle = glm::translate (glm::vec3(Obstacles[i].x, Obstacles[i].y, Obstacles[i].z));
        Matrices.model = translateRectangle;
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(Obstacles[i].obstacle);
    }
  }
  // Increment angles
  float increments = 1;
  
  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle ();
	createPlayer ();
    createObstacle ();
    
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
void playeradventure ()
{
    if(state1==1)
        {
            //if(dir==1)
            //    x+=t;
            //else if(dir==2)
            z-=t;
            //else if(dir==3)
            //    x-=t;
            //else if(dir==4)
            //    z+=t;
            dir=2;
            if(dir==1)
            {
                player_rot = 90;
                dir=2;
            }
            else if(dir==3)
            {
                player_rot = -90;
                dir=2;
            }
            else if(dir==4)
            {
                player_rot = 180;
                dir=2;
            }
        }
        if(state2==1)
        {
            z+=t;
            dir=4;
            if(dir==1)
            {
                player_rot = -90;
                dir=4;
            }
            if(dir==2)
            {
                player_rot = 180;
                dir=4; 
            }
            if(dir==3)
            {
                player_rot = 90;
                dir=4;
            }
        }
        if(state3==1)
        {
            x-=t;
            dir=3;
            if(dir==1)
            {
                player_rot = 180;
                dir=3;
            }
            if(dir==2)
            {
                player_rot = 90;
                dir=3;
            }
            if(dir==4)
            {
                player_rot = -90;
                dir=3;
            }
        }
        if(state4==1)
        {
            x+=t; 
            dir=1;
            if(dir==2)
            {
                player_rot = -90;
                dir=1;
            }     
            if(dir==3)
            {
                player_rot = 180;
                dir=1;
            }
            if(dir==4)
            {
                player_rot = 90;
                dir=1;
            }
        }    
} 
void playerheaddir ()
{
    if(state1==1)
        {
            z-=t;
            
        }
        if(state2==1)
        {
            z+=t;
            
        }
        if(state3==1)
        {
            x-=t;
            
        }
        if(state4==1)
        {
            x+=t; 
            
        }
}  
void helicopterview ()
{
    if(state5==1)
        angle_xz++;
    if(state6==1)
        angle_xz--;
    if(state7==1)
        camera_y-=0.2;
    if(state8==1)
        camera_y+=0.2;
}
void scroll(GLFWwindow* window, double x,double y)
{
    if(y>0)
        zoom*=1.1;
    else if(y<0&&zoom>0)
        zoom*=0.9;
    else
        zoom=1;
    //cout<<y<<endl;

}
void Obstacleblock ()
{
    for(int i=0;i<200;i++)
        {
            if(b[i]==1&&space==0)
            {
                if(z<=Obstacles[i].z+0.5&&z>=Obstacles[i].z-0.5)
                {
                    if(x-Obstacles[i].x<=0.5&&x-Obstacles[i].x>=0.25)
                        x=Obstacles[i].x+0.5;
                    else if(Obstacles[i].x-x<=0.5&&Obstacles[i].x-x>=0.25)
                        x=Obstacles[i].x-0.5;
                }
                if(x<=Obstacles[i].x+0.5&&x>=Obstacles[i].x-0.5)
                {
                    if(z-Obstacles[i].z<=0.5&&z-Obstacles[i].z>=0.25)
                        z=Obstacles[i].z+0.5;
                    else if(Obstacles[i].z-z<=0.5&&Obstacles[i].z-z>=0.25)
                        z=Obstacles[i].z-0.5;
                }
            }
        }

}
void Movingpilesblock ()
{
    for(int i=0;i<200;i++)
        {
            if(a[i]==1&&space==0)
            {
                if(z<=Rectangles[i].z+0.5&&z>=Rectangles[i].z-0.5)
                {
                    if(x-Rectangles[i].x<=0.75&&x-Rectangles[i].x>=0.5)
                        x=Rectangles[i].x+0.75;
                    else if(Rectangles[i].x-x<=0.75&&Rectangles[i].x-x>=0.5)
                        x=Rectangles[i].x-0.75;
                }
                if(x<=Rectangles[i].x+0.75&&x>=Rectangles[i].x-0.75)
                {
                    if(z-Rectangles[i].z<=0.75&&z-Rectangles[i].z>=0.5)
                        z=Rectangles[i].z+0.75;
                    else if(Rectangles[i].z-z<=0.75&&Rectangles[i].z-z>=0.5)
                        z=Rectangles[i].z-0.75;
                }
            }
        }
        
}
void Jump ()
{
    if(space==1)
        {
            if(y<2.8 && st1==0)
            {
                y+=0.1;
                st1=0;
            }
            else if(y>2)
            {
                st1=1;
                y-=0.1;
            }
            if(y>2 && st1==0)
            {
                y+=0.1;
                st1=0;
            }
            else if(y>2)
            {
                st1=1;
                y-=0.1;
            }
            if(y<=2 && st1==1)
            {
                space=0;
                st1=0;
            }

        }
        
}
void Pitfall ()
{
    if(space==0 && (((x>=1.75)&&(x<=2.25)) && ((z>=-1.25)&&(z<=-0.75)))||(((x>=-0.25)&&(x<=0.25)) && ((z>=0.75)&&(z<=1.25)))||(((x>=-3.25)&&(x<=-2.75)) && ((z>=2.75)&&(z<=3.25)))||(((x>=-2.25)&&(x<=-1.75)) && ((z>=4.75)&&(z<=5.25)))||(((x>=3.75)&&(x<=4.25)) && ((z>=-4.25)&&(z<=-3.75)))||(((x>=-2.25)&&(x<=-1.75)) && ((z>=-1.25)&&(z<=-0.75))))
        {
            y-=0.5;
            space=0;
        }
        //else if(space==0)
        //    y=2;
        if(y<=0)
        {
            glfwTerminate();
            exit(EXIT_SUCCESS);            
        }
}
void Win ()
{
    if(x>=6&&z<=-7)
        {
            cout<<"Game Won\n";
            glfwTerminate();
            exit(EXIT_SUCCESS);   
        }
}
void Boundary ()
{
    if(z<=-7.25)
        z=-7.25;
    if(z>=6.25)
        z=6.25;
    if(x<=-7.25)
        x=-7.25;
    if(x>=6.25)
        x=6.25;
}
void Pilesmotion ()
{
    for (int i = 0; i < 200; i++)
        {
            if(a[i]==1)
            {
                if(Rectangles[i].y<0.5 && st==0)
                {
                    Rectangles[i].y+=0.05;
                    st=0;
                }
                else if(Rectangles[i].y>-0.5)
                {
                    st=1;
                    Rectangles[i].y-=0.05;
                }
                if(Rectangles[i].y>-0.5 && st==0)
                {
                    Rectangles[i].y+=0.05;
                    st=0;
                }
                else if(Rectangles[i].y>-0.5)
                {
                    st=1;
                    Rectangles[i].y-=0.05;
                }
                if(Rectangles[i].y<=-0.5)
                    st=0;
            }
        }
}
int main (int argc, char** argv)
{
	int width = 1600;
	int height = 900;
    
    //int b[500]={0};
    
    //double xpos,ypos;

    GLFWwindow* window = initGLFW(width, height);

    for(int i=0;i<200;i++)
    {
        if((rand () % 10)==0)
        {
            if(i!=13&&i!=151)
                a[i]=1;
        }
        else if(rand () % 15==0)
        {
            if(i!=13&&i!=151)
                b[i]=1;
        }
    }

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

        /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.065) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
        
        glfwGetCursorPos (window, &xpos, &ypos);
        xpos=-77+(float)154.0/width*xpos;

        ypos=-40+(float)80.0/height*ypos;

        ypos*=-1;

        //float x = -70.0;

        //float y = -5.0;
        Obstacleblock ();

        Movingpilesblock ();

        Jump ();

        Pitfall ();

        Win ();
        
        Boundary ();

        Pilesmotion ();
        //cannon_rotation = atan2 (ypos-y,xpos-x) * 180 / M_PI;
        if(view==2)
            playerheaddir ();
        else if(view==1)
            playeradventure ();
        else if(view==5)
        {
            playerheaddir ();
            helicopterview ();
            glfwSetScrollCallback (window, scroll);
        }
        else
            playerheaddir ();
        
        
        
          //cout<<x<<" "<<z<<endl;
        
        
        
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
