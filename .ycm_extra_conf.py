import os

project_root = os.path.abspath(os.path.dirname(__file__))

# YouCompleteMe default project settings
def Settings( **kwargs ):
  return {
      'flags': ['-O0', '-std=gnu11'],
      'include_paths_relative_to_dir': project_root,
      }

